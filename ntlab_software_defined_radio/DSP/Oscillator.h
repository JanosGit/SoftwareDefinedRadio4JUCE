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

#include "../OpenCL2/clArray.h"

namespace ntlab
{
    /**
     * A simple oscillator using std::sin / std::cos functions to create a continous wave. If you want to go for
     * performance you will probably like to implement some more sophisticated algorithm yourself.
     *
     * The oscillator is aware of the SDR's center frequency if you attached it as a tune change listener to your
     * SDRIOHardwareEngine instance. This means, the frequency you set will be the true RF output frequency.
     */
    class Oscillator : public SDRIOHardwareEngine::TuneChangeListener
    {

    public:
        /**
         * Creates an Oscillator instance. The number of channels passed must match the number of channels of the buffer
         * passed to fillNextSampleBuffer. Each channel can have its own frequency, phase shift, etc.
         */
        Oscillator (const int numChannels);

        /**
         * Sets the frequency the oscillator outputs. If you attached it as a tune change listener to an
         * SDRIOHardwareEngine, this will be the true rf frequency outputted on the sdr. You can pass
         * SDRIOEngine::allChannels as channel value if you want to set all channels.
         */
        bool setFrequencyHz (double newFrequencyInHz, int channel);

        /**
         * Returns the frequency this channel currently outputs. If you attached it as a tune change listener to an
         * SDRIOHardwareEngine, this will be the true rf frequency outputted on the sdr.
         */
        double getFrequencyHz (int channel);

        /**
         * Sets a phase shift in radians for this channel. You can pass SDRIOEngine::allChannels as channel value if
         * you want to set all channels.
         */
        void setPhaseShift (double newPhaseShift, int channel);

        /** Returns the phase shift in radians for this channel. */
        double getPhaseShift (int channel);

        /** Sets the sample rate. Always call this before the first call to fillNextSampleBufer! */
        void setSampleRate (double newSampleRate);

        /** Returns the sample rate the oscillator is currently expecting */
        double getSampleRate();

        /**
         * Sets the amplitude as a linear gain value for this channel. You can pass SDRIOEngine::allChannels as channel
         * value if you want to set all channels.
         */
        void setAmplitude (double newAmplitude, int channel);

        /** Returns the amplitude for this channel as linear gain value */
        double getAmplitude (int channel);

#if NTLAB_USE_CL_DSP
		/**
		 * Fills a sample buffer with the next block of continuous samples. This is meant to be called on a regular
		 * basis from within SDRIODeviceCallback::processRFSampleBlock.
		 *
		 * The supported buffer types are CLSampleBufferRealx or CLSampleBufferComplex with float samples.
		 * 
		 * !!Attention: This function expects the buffer passed to be in an unmapped state!!
		 */
        template <typename BufferType>
        void fillNextSampleBuffer (BufferType& buffer)
        {
            static_assert (IsSampleBuffer<BufferType, float>::cl(), "Oscillator: Unsupported buffer type");
            // you should set the sampleRate through setSampleRate or by passing an SDREngine to the constructor
            jassert (currentSampleRate > 0);
            jassert (buffer.getNumChannels() == numChannels);

			// This function expects the buffer passed to be in an unmapped state
			jassert(buffer.isCurrentlyMapped() == false);

            const int numSamples  = buffer.getNumSamples();

            auto& ampl = amplitude.unmap();

            for (int channel = 0; channel < numChannels; ++channel)
                phaseAndAngle[channel] = phase[channel] + currentAngle[channel];

            auto& paa = phaseAndAngle.unmap();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                double newAngle = currentAngle[channel] + numSamples * angleDelta[channel];
                newAngle = std::fmod (newAngle, juce::MathConstants<double>::twoPi);

                currentAngle.set (channel, newAngle);
            }

            auto& ad   = angleDelta.unmap();

            complexOscillatorKernel.setArg (0, buffer.getCLBuffer());
            complexOscillatorKernel.setArg (1, buffer.getCLChannelList());
            complexOscillatorKernel.setArg (2, paa);
            complexOscillatorKernel.setArg (3, ad);
            complexOscillatorKernel.setArg (4, ampl);

            // wait for unmap operations to finish
            clQueue.finish();

			auto kernelStartTime = juce::Time::getHighResolutionTicks();

            if constexpr (BufferType::isComplex())
            {
                clQueue.enqueueNDRangeKernel (complexOscillatorKernel, cl::NullRange, cl::NDRange (numChannels, numSamples));
            }
            else
            {
                clQueue.enqueueNDRangeKernel (realOscillatorKernel, cl::NullRange, cl::NDRange (numChannels, numSamples));
            }

            // wait for kernel to finish
            clQueue.finish();

			auto kernelFinishTime = juce::Time::getHighResolutionTicks();

            amplitude    .map();
            phaseAndAngle.map();
            angleDelta   .map();

			kernelTime += kernelFinishTime - kernelStartTime;

            // wait for the map operations to finish
            clQueue.finish();
        }

		void printTimeResults(juce::int64 numCallbacks)
		{
			auto timePerKernel = juce::String(juce::Time::highResolutionTicksToSeconds(kernelTime / numCallbacks));
			juce::Logger::writeToLog("Time per Kernel: " + timePerKernel + "sec");
			kernelTime = 0;
		}
#else
        /**
         * Fills a sample buffer with the next block of continuous samples. This is meant to be called on a regular
         * basis from within SDRIODeviceCallback::processRFSampleBlock.
         *
         * The supported buffer types are SampleBufferReal, CLSampleBufferReal, SampleBufferComplex or
         * CLSampleBufferComplex with float, double or int32 or int16 samples. Note that in case of integer samples you
         * should match the amplitude value to create a matching value range as the default value range will be -1.0 to
         * 1.0
         */
        template <typename BufferType>
        void fillNextSampleBuffer (BufferType& buffer)
        {
            static_assert (IsSampleBuffer<BufferType>::complexOrReal(), "Oscillator: Unsupported buffer type");
            // you should set the sampleRate through setSampleRate or by passing an SDREngine to the constructor
            jassert (currentSampleRate > 0);
            jassert (buffer.getNumChannels() == numChannels);

            const int numSamples = buffer.getNumSamples();
            auto bufferWritePtrs = buffer.getArrayOfWritePointers();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                double&         ca = currentAngle.begin()[channel];
                const double&   ad = angleDelta.getUnchecked (channel);
                const double&  pha = phase     .getUnchecked (channel);
                const double& ampl = amplitude .getUnchecked (channel);

                for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
                {
                    if constexpr (BufferType::isComplex())
                    {
                        bufferWritePtrs[channel][sampleIdx] = BufferType::castToSampleType (std::cos (ca + pha) * ampl, std::sin (ca + pha) * ampl);
                    }
                    else
                    {
                        bufferWritePtrs[channel][sampleIdx] = BufferType::castToSampleType (std::sin (ca + pha) * ampl);
                    }
                    ca += ad;

                    // limit the angle to +/- pi to avoid overflows and performance impacts with big angle values
                    if (ca > juce::MathConstants<double>::pi)
                        ca -= juce::MathConstants<double>::twoPi;
                    if (ca < -juce::MathConstants<double>::pi)
                        ca += juce::MathConstants<double>::twoPi;
                }
            }
        }
#endif

        /** TuneChangeListener member function, you won't need to call it yourself */
        void txCenterFreqChanged (double newTxCenterFreq, int channel) override;

        /** TuneChangeListener member function, you won't need to call it yourself */
        void txBandwidthChanged (double newBandwidth, int channel) override;

    private:
#if NTLAB_USE_CL_DSP
        using ArrayType = CLArray<cl_float, juce::CriticalSection>;
        const cl::Context&      clContext;
        const cl::CommandQueue& clQueue;
#ifdef OPEN_CL_INTEL_FPGA
        cl::Program&      clProgram;
#else
        cl::Program       clProgram;
#endif
        cl::Kernel        complexOscillatorKernel;
        cl::Kernel        realOscillatorKernel;
        CLArray<cl_float> phaseAndAngle;

		juce::int64 kernelTime = 0;
#else
        using ArrayType = juce::Array<double>;
#endif

        const int numChannels;
        double currentSampleRate = 0.0;

        juce::Array<double> rfFrequency;   // the frequency that will be physically broadcasted
        juce::Array<double> sdrCenterFreq; // will create a certain frequency shift from the if freq
        juce::Array<double> ifFrequency;   // the frequency of the digital signal generated

        juce::Array<double> phase;
        juce::Array<double> currentAngle;
        ArrayType angleDelta;
        ArrayType amplitude;

        void updateAngleDelta();
    };
}