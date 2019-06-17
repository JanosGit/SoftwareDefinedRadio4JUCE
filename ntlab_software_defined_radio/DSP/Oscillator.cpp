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

#include "Oscillator.h"
#include "../OpenCL2/ntlab_OpenCLHelpers.h"
#include "../OpenCL2/SharedCLDevice.h"

namespace ntlab
{
#if NTLAB_USE_CL_DSP
    Oscillator::Oscillator (const int nChannels)
     : clContext     (SharedCLDevice::getInstance()->getContext()),
       clQueue       (SharedCLDevice::getInstance()->getCommandQueue()),
       phaseAndAngle (nChannels, clContext, clQueue, CL_MEM_READ_ONLY),
       numChannels   (nChannels),
       angleDelta    (nChannels, clContext, clQueue, CL_MEM_READ_ONLY),
       amplitude     (nChannels, clContext, clQueue, CL_MEM_READ_ONLY)
    {
        rfFrequency  .resize (nChannels);
        sdrCenterFreq.resize (nChannels);
        ifFrequency  .resize (nChannels);
        phase        .resize (nChannels);
        currentAngle .resize (nChannels);

        rfFrequency  .fill (0.0);
        sdrCenterFreq.fill (0.0);
        ifFrequency  .fill (0.0);
        phase        .fill (0.0);
        currentAngle .fill (- juce::MathConstants<double>::pi);
        angleDelta   .fill (0.0);
        amplitude    .fill (1.0);

        auto* clDevice = SharedCLDevice::getInstance();

#ifndef OPENCL_INTEL_FPGA
        const std::string clSources =
#include "Oscillator.cl"
        ;

        clProgram = clDevice->createProgramForDevice (clSources);
#else
        clProgram = clDevice->getFPGABinaryProgram();
#endif

        cl_int err;
        complexOscillatorKernel = cl::Kernel (clProgram, "oscillatorFillNextSampleBufferComplex", &err);
        if (err != CL_SUCCESS)
            throw CLException ("Error creating complex valued oscillator kernel", err);

        realOscillatorKernel    = cl::Kernel (clProgram, "oscillatorFillNextSampleBufferReal", &err);
        if (err != CL_SUCCESS)
            throw CLException ("Error creating real oscillator kernel", err);
    }

#else

    Oscillator::Oscillator (const int nChannels) : numChannels (nChannels)
    {
        rfFrequency.resize   (nChannels);
        sdrCenterFreq.resize (nChannels);
        ifFrequency.resize   (nChannels);
        phase.resize         (nChannels);
        currentAngle.resize  (nChannels);
        angleDelta.resize    (nChannels);
        amplitude.resize     (nChannels);

        rfFrequency  .fill (0.0);
        sdrCenterFreq.fill (0.0);
        ifFrequency  .fill (0.0);
        phase        .fill (0.0);
        currentAngle .fill (- juce::MathConstants<double>::pi);
        angleDelta   .fill (0.0);
        amplitude    .fill (1.0);
    }
#endif

    bool Oscillator::setFrequencyHz (double newFrequencyInHz, int channel)
    {
        if (channel == SDRIOEngine::allChannels)
            rfFrequency.fill (newFrequencyInHz);
        else
            rfFrequency.set (channel, newFrequencyInHz);

        updateAngleDelta();
        return true;
    }

    double Oscillator::getFrequencyHz (int channel) {return rfFrequency[channel]; }

    void Oscillator::setPhaseShift (double newPhaseShift, int channel)
    {
        if (channel == SDRIOEngine::allChannels)
            phase.fill (newPhaseShift);
        else
            phase.set (channel, newPhaseShift);
    }

    double Oscillator::getPhaseShift (int channel) {return phase[channel]; }

    void Oscillator::setSampleRate (double newSampleRate)
    {
        currentSampleRate = newSampleRate;
        updateAngleDelta();
    }

    double Oscillator::getSampleRate () {return currentSampleRate; }

    void Oscillator::setAmplitude (double newAmplitude, int channel) {amplitude.set (channel, newAmplitude); }

    double Oscillator::getAmplitude (int channel) {return amplitude[channel]; }

    void Oscillator::updateAngleDelta()
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            ifFrequency.set (channel, rfFrequency[channel] - sdrCenterFreq[channel]);
            auto cyclesPerSample = std::fmod (ifFrequency[channel] / currentSampleRate, 1.0);
            angleDelta.set (channel, cyclesPerSample * juce::MathConstants<double>::twoPi);
        }
    }

    void Oscillator::txCenterFreqChanged (double newTxCenterFreq, int channel)
    {
        if (channel == SDRIOEngine::allChannels)
            sdrCenterFreq.fill (newTxCenterFreq);
        else
            sdrCenterFreq.set (channel, newTxCenterFreq);

        updateAngleDelta();
    }

    void Oscillator::txBandwidthChanged (double newBandwidth, int channel)
    {
#if JUCE_DEBUG
        // if you hit this assert, the oscillator frequency is set higher than the current tx bandwith of your SDR
        // hardware supports
        if (channel == SDRIOEngine::allChannels)
            for (auto& iff : ifFrequency)
                jassert (newBandwidth >= iff);
        else
            jassert (newBandwidth >= ifFrequency[channel]);
#endif
    }
}