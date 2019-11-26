/*
This file is part of SoftwareDefinedRadio4JUCE.

SoftwareDefinedRadio4JUCE is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License
as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

SoftwareDefinedRadio4JUCE is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with SoftwareDefinedRadio4JUCE. If not, see <http://www.gnu.org/licenses/>.
*/

#include <juce_dsp/juce_dsp.h>

#include "GNSSAquisition.h"
#include "GPSCaCodeGenerator.h"
#include "../OpenCL2/ntlab_OpenCLHelpers.h"
#include "../OpenCL2/SharedCLDevice.h"
#include "../OpenCL2/clArray.h"

namespace ntlab
{
    GNSSAquisition::GNSSAquisition ()
     : juce::Thread ("GNSSAcquisitionThread"),
       context                    (SharedCLDevice::getInstance()->getContext()),
       queue                      (context),
#ifdef OPEN_CL_INTEL_FPGA
       clProgram                  (SharedCLDevice::getInstance()->getFPGABinaryProgram()),
#endif
       twiddleTable               (createTwiddleTable (context)),
       caCodes                    (numCACodes,     caCodeLength),
       caCodesUpsampled           (numCACodes,     fftSize, queue, context),
       caCodesUpsampledFreqDomain (numCACodes,     fftSize, queue, context),
       inputSignal                (1,              fftSize, queue, context),
       mixedInputSignals          (numFreqOffsets, fftSize, queue, context),
       intermediateResults        (numFreqOffsets, fftSize, queue, context),
       acquisitionSpecBuffer      (numFreqOffsets, fftSize, queue, context),
       acquisitionSpecMaxPositions(numFreqOffsets, context, queue, CL_MEM_WRITE_ONLY),
       acquisitionSpecMaxValues   (numFreqOffsets, context, queue, CL_MEM_WRITE_ONLY),
       acquisitionSpecMeanValues  (numFreqOffsets, context, queue, CL_MEM_WRITE_ONLY)
    {
#ifndef OPEN_CL_INTEL_FPGA
        const std::string clSources =
#include "GNSSAquisition.cl"
        ;

        clProgram = SharedCLDevice::getInstance()->createProgramForDevice (clSources);
#endif

        inputSignal.setNumSamples (0);
        startThread (7);

#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        if (loggingResults.openedOk())
        {
            loggingResults.setPosition (0);
            loggingResults.truncate();
            loggingResults.writeString ("ProcessingStep,usPerSample,load,numSampsDropped,numAcquisitions\n");
        }
        else
            throw std::runtime_error ("Error opening log file");
#endif
    }

    GNSSAquisition::~GNSSAquisition()
    {
        stopThread (20000);
#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        loggingResults.writeString ("NumAcquisitions,,,," + juce::String (numAcquisitionsPerformed) + "\n");
        loggingResults.flush();
        juce::String loggingFilePath = loggingFile.getFullPathName();

        std::cout << "Performance logging file located at " << loggingFilePath << std::endl;

        //juce::String cmd = "cat " + loggingFilePath;
        //std::system (cmd.toRawUTF8());
#endif
    }

    void GNSSAquisition::setupKernels()
    {
        cl_int err;
        mixKernel = cl::Kernel (clProgram, "gnssMixInput", &err);
        if (err != CL_SUCCESS)
            throw CLException ("Error creating gnssMixInput kernel", err);

        acquisitionKernel = cl::Kernel (clProgram, "gnssAcquisition", &err);
        if (err != CL_SUCCESS)
            throw CLException ("Error creating gnssAquisition kernel", err);

        err = mixKernel.setArg (0, inputSignal.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssMixInput kernel arg 0", err);

        err = mixKernel.setArg (1, mixedInputSignals.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssMixInput kernel arg 1", err);

        err = mixKernel.setArg (2, twiddleTable);
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssMixInput kernel arg 2", err);

        err = acquisitionKernel.setArg (0, mixedInputSignals.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 0", err);

        err = acquisitionKernel.setArg (1, caCodesUpsampledFreqDomain.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 1", err);

        err = acquisitionKernel.setArg (2, intermediateResults.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 2", err);

        err = acquisitionKernel.setArg (3, twiddleTable);
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 3", err);

        err = acquisitionKernel.setArg (5, acquisitionSpecBuffer.getCLBuffer());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 5", err);

        err = acquisitionKernel.setArg (6, acquisitionSpecMaxPositions.unmap());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 6", err);

        err = acquisitionKernel.setArg (7, acquisitionSpecMaxValues.unmap());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 7", err);

        err = acquisitionKernel.setArg (8, acquisitionSpecMeanValues.unmap());
        if (err != CL_SUCCESS)
            throw CLException ("Error setting gnssAcquisition kernel arg 8", err);

        intermediateResults.unmapHostMemory();
        acquisitionSpecBuffer.unmapHostMemory();
    }

    void GNSSAquisition::initCACodeLUTs()
    {
        // Generation of the CA Codes
        GPSCaCodeGenerator caCodeGenerator;
        for (int code = 0; code < numCACodes; ++code)
        {
            caCodeGenerator.computeCaCode (caCodes.getWritePointer (code), code);
            // Default: Acquire all codes except the last two that are not broadcasted
            if (code < (numCACodes - 2))
                caCodesToAcquire.push_back (code);
        }

        // Upsampling and transformation of the ca codes
        const double speedRatio = 0.062501527407806;
        const int numInterpolatedSamples = static_cast<int> (caCodeLength / speedRatio);

        juce::dsp::FFT fft (fftOrder);

        caCodesUpsampled          .mapHostMemory();
        caCodesUpsampledFreqDomain.mapHostMemory();

        caCodesUpsampled.clearBufferRegion();

        for (int codeIdx = 0; codeIdx < numCACodes; ++codeIdx)
        {
            auto* sourcePtr         = caCodes                   .getReadPointer  (codeIdx);
            auto* upsampledDestPtr  = caCodesUpsampled          .getWritePointer (codeIdx);
            auto* freqDomainDestPtr = caCodesUpsampledFreqDomain.getWritePointer (codeIdx);

            for (int s = 0; s < numInterpolatedSamples; ++s)
                upsampledDestPtr[s] = sourcePtr[static_cast<int> (s * speedRatio)];

            juce::FloatVectorOperations::copy   (reinterpret_cast<float*> (freqDomainDestPtr), upsampledDestPtr, fftSize);
            fft.performRealOnlyForwardTransform (reinterpret_cast<float*> (freqDomainDestPtr));

            VectorOperations::permuteInBitReversedOrder<fftOrder> (freqDomainDestPtr);
        }

        caCodesUpsampled          .unmapHostMemory();
        caCodesUpsampledFreqDomain.unmapHostMemory();
    }

    void GNSSAquisition::run()
    {
        setupKernels();
        initCACodeLUTs();

        // never needs to be mapped, only used by the kernel
        // todo: maybe replace by a simple cl::Buffer
        mixedInputSignals.unmapHostMemory();

#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        mixAndFFTTime  .processingStarts (targetSampleRate);
        acquisitionTime.processingStarts (targetSampleRate);
#endif

        // Wait for all async cl tasks to finish
        queue.finish();

        while (true)
        {
            wait (-1);
            if (threadShouldExit())
                break;

            inputSignal.unmapHostMemory();
            queue.finish();

            performAcquisition();
        }

        queue.finish();
    }

    void GNSSAquisition::setSampleRate (double newSampleRate)
    {
#ifdef OPEN_CL_INTEL_FPGA
        // You have to make sure that your frontend generates the exact sample rate needed - resampling on the tiny
        // ARM of your FPGA SoC is a really bad idea so the resampler is disabled for this setup
        jassert (newSampleRate == targetSampleRate);
#else
        if (newSampleRate == targetSampleRate)
        {
            interpolatorRatio = 1.0;
            needsSampleRateConversion = false;
            return;
        }

        interpolatorRatio = newSampleRate / targetSampleRate;
        needsSampleRateConversion = true;
        resampler.reset();
#endif
    }

    void GNSSAquisition::performAcquisition()
    {
        {
#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
            auto sm = mixAndFFTTime.startScopedMeasurement (fftSize);
#endif

            // First mix the signal to the different frequency offsets and perform an FFT on the results
            auto err = queue.enqueueNDRangeKernel (mixKernel, cl::NullRange, cl::NDRange (numFreqOffsets));
            if (err != CL_SUCCESS)
                std::cerr << OpenCLHelpers::getErrorString (err);
            queue.finish();

        }

        // The input buffer won't be needed anymore at this point and can be unmapped and released so that new samples
        // can be filled in
        inputSignal.mapHostMemory();
        inputSignal.setNumSamples (0);
        inputSignalLock.unlock();

#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        int numSamplesFrac = fftSize / caCodesToAcquire.size();
#endif

        for (auto caCode : caCodesToAcquire)
        {
            float maxValue  = -std::numeric_limits<float>::max();
            float meanValue = 0.0f;
            int   freqOffsetOfPeakInHz;
            float codeOffsetOfPeakInBins;

            {
#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
                auto sm = acquisitionTime.startScopedMeasurement (numSamplesFrac);
#endif
                acquisitionKernel.setArg (4, caCode);
                queue.finish();

                auto err = queue.enqueueNDRangeKernel (acquisitionKernel, cl::NullRange, cl::NDRange (numFreqOffsets));
                if (err != CL_SUCCESS)
                    std::cerr << OpenCLHelpers::getErrorString (err);

                queue.finish();


                acquisitionSpecMaxPositions.map (CL_FALSE, CL_MAP_READ);
                acquisitionSpecMaxValues   .map (CL_FALSE, CL_MAP_READ);
                acquisitionSpecMeanValues  .map (CL_FALSE, CL_MAP_READ);
                queue.finish();
                acquisitionSpecBuffer.mapHostMemory (CL_FALSE);

                int codeShiftIdxOfMax = 0;
                int freqShiftIdxOfMax = 0;

                for (int i = 0; i < numFreqOffsets; ++i)
                {
                    float v = acquisitionSpecMaxValues[i];
                    if (v > maxValue)
                    {
                        maxValue = v;
                        codeShiftIdxOfMax = acquisitionSpecMaxPositions[i];
                        freqShiftIdxOfMax = i;
                    }

                    meanValue += (acquisitionSpecMeanValues[i] / fftSize);
                }

                acquisitionSpecMaxPositions.unmap();
                acquisitionSpecMaxValues   .unmap();
                acquisitionSpecMeanValues  .unmap();

                meanValue /= numFreqOffsets;

                freqOffsetOfPeakInHz = getFreqOffsetInHz (freqShiftIdxOfMax);
                codeOffsetOfPeakInBins = ((static_cast<float> (fftSize) / codeShiftIdxOfMax) - 0.5f) * 511.5f;

                queue.finish();
            }

            acquisitionSpecCallback (acquisitionSpecBuffer, caCode, maxValue, meanValue, freqOffsetOfPeakInHz, codeOffsetOfPeakInBins);

            acquisitionSpecBuffer.unmapHostMemory();
        }
#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        ++numAcquisitionsPerformed;
#endif
    }

    const juce::Range<float> GNSSAquisition::getFreqOffsetRange()
    {
        return {(float)getFreqOffsetInHz (0), (float)getFreqOffsetInHz (numFreqOffsets)};
    }

    const int GNSSAquisition::getFreqOffsetInHz (int offsetIdx)
    {
        const int minFreq = (-numFreqOffsets / 2) * freqSpacingHz + (freqSpacingHz / 2);
        return minFreq + offsetIdx * freqSpacingHz;
    }
}