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

#pragma once

#define GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT

#include <ntlab_software_defined_radio/OpenCL2/cl2.hpp>
#include <juce_audio_basics/juce_audio_basics.h>
#include <ntlab_software_defined_radio/PerformanceMeasurement/ProcessingTimeMeasurement.h>

namespace ntlab
{
    class GNSSAquisition : private juce::Thread
    {
    public:
        GNSSAquisition();
        ~GNSSAquisition();

        void setSampleRate (double newSampleRate);

        template <typename BufferType>
        void processNextSampleBuffer (const BufferType& bufferToProcess, const cl_int caCodeToAquire = -1)
        {
            auto numInterpolatedSamplesAvailable = static_cast<int> (bufferToProcess.getNumSamples() / interpolatorRatio);

            if (inputSignalLock.try_lock())
            {
                auto numSamplesInInputBuffer = inputSignal.getNumSamples();
                auto numSamplesToAppend = std::min (fftSize - numSamplesInInputBuffer, numInterpolatedSamplesAvailable);

                if (needsSampleRateConversion)
                {
                    auto numSamplesConsumed = resampler.process (interpolatorRatio,
                                                                 bufferToProcess.getReadPointer (0),
                                                                 inputSignal.getWritePointer (0) + numSamplesInInputBuffer,
                                                                 numSamplesToAppend);
                    jassert (numSamplesConsumed <= bufferToProcess.getNumSamples());
                    juce::ignoreUnused (numSamplesConsumed);
                }
                else
                {
                    bufferToProcess.copyTo (inputSignal, numSamplesToAppend, 1, 0, numSamplesInInputBuffer);
                }

                inputSignal.incrementNumSamples (numSamplesToAppend);

                if (inputSignal.getNumSamples() == fftSize)
                {
                    // let the acquisition thread handle the rest of the work. The lock will be released by the thread
                    notify();
                    return;
                }

                inputSignalLock.unlock();
            }
            else
            {
                // Not getting the lock means that we drop some input samples as the acquisition thread is still working
                // on the buffer. This is no big problem, however we better clear our resampler state as the data won't
                // be contignous anymore
                resampler.reset();

#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
                numSamplesDropped += numInterpolatedSamplesAvailable;
#endif
            }
        }

        std::function<void(CLSampleBufferReal<uint8_t>& acqSpec,
                           int                          caCodeIdx,
                           float                        maxValue,
                           float                        meanValue,
                           float                        freqOffsetOfMaxValue,
                           float                        codeOffsetOfMAxValue)> acquisitionSpecCallback;


        static const int fftOrder           = 14;
        static const int fftSize            = 1 << fftOrder;
        static const int numFreqOffsets     = 28;
        static const int numCACodes         = 37;
        static const int freqSpacingHz      = 500;
        static const int caCodeLength       = 1023;
        static constexpr double targetSampleRate {16.3676e6};

        static const juce::Range<float> getFreqOffsetRange();

    private:
        const cl::Context&      context;
        // The thread runs an own queue
        cl::CommandQueue        queue;

#ifdef OPEN_CL_INTEL_FPGA
        const cl::Program& clProgram;
#else
        cl::Program clProgram;
#endif
        cl::Kernel mixKernel;
        cl::Kernel acquisitionKernel;

        std::vector<cl_int> caCodesToAcquire;

        const cl::Buffer twiddleTable;
        SampleBufferReal     <float> caCodes;
        CLSampleBufferReal   <float> caCodesUpsampled;
        CLSampleBufferComplex<float> caCodesUpsampledFreqDomain;
        CLSampleBufferComplex<float> inputSignal;
        CLSampleBufferComplex<float> mixedInputSignals;
        CLSampleBufferComplex<float> intermediateResults;
        CLSampleBufferReal<uint8_t>  acquisitionSpecBuffer;
        CLArray<int>                 acquisitionSpecMaxPositions;
        CLArray<float>               acquisitionSpecMaxValues;
        CLArray<float>               acquisitionSpecMeanValues;

        std::mutex inputSignalLock;

        bool needsSampleRateConversion = false;
        juce::LagrangeResampler<std::complex<float>, float> resampler;
        double interpolatorRatio = 1.0;

#ifdef GNSS_ACQUISITION_ENABLE_PERFORMANCE_MEASUREMENT
        const juce::File loggingFile {juce::File::getSpecialLocation (juce::File::currentExecutableFile).getSiblingFile ("GNSSAcqPerformanceLoggingResults.csv")};
        juce::FileOutputStream loggingResults {loggingFile};
        std::atomic<int64_t> numSamplesDropped = 0;
        int                  numAcquisitionsPerformed = 0;
        ProcessingTimeMeasurement mixAndFFTTime {5 * fftSize, [this] (double us, double load, juce::int64)
                                                               {
                                                                   using Str = juce::String;

                                                                   Str line = "mixFFT," + Str (us) + "," + Str (load) + "," + Str (numSamplesDropped.load()) + "\n";
                                                                   loggingResults.writeString (line);

                                                                   numSamplesDropped.store (0);
                                                               }};

        ProcessingTimeMeasurement acquisitionTime {5 * fftSize, [this] (double us, double load, juce::int64)
                                                                 {
                                                                     using Str = juce::String;

                                                                     Str line = "acquisition," + Str (us) + "," + Str (load) + "\n";
                                                                     loggingResults.writeString (line);
                                                                 }};
#endif

        void setupKernels();
        void initCACodeLUTs();
        void performAcquisition();

        void run() override;

        static const cl::Buffer createTwiddleTable (const cl::Context& context);
        static const int getFreqOffsetInHz (int offsetIdx);
    };
}