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

            const int numSamples  = buffer.getNumSamples();
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

        /** TuneChangeListener member function, you won't need to call it yourself */
        void txCenterFreqChanged (double newTxCenterFreq, int channel) override;

        /** TuneChangeListener member function, you won't need to call it yourself */
        void txBandwidthChanged (double newBandwidth, int channel) override;
        
    private:
        const int numChannels;
        double currentSampleRate = 0.0;

        juce::Array<double> rfFrequency;   // the frequency that will be physically broadcasted
        juce::Array<double> sdrCenterFreq; // will create a certain frequency shift from the if freq
        juce::Array<double> ifFrequency;   // the frequency of the digital signal generated

        juce::Array<double> phase;
        juce::Array<double> currentAngle;
        juce::Array<double> angleDelta;
        juce::Array<double> amplitude;

        void updateAngleDelta();
    };
}