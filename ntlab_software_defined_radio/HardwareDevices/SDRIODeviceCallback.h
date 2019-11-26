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

#include "../SampleBuffers/SampleBuffers.h"

namespace ntlab
{

#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
    using OptionalCLSampleBufferComplexFloat = CLSampleBufferComplex<float>;
#else
    using OptionalCLSampleBufferComplexFloat = SampleBufferComplex<float>;
#endif

    /**
     * A pure virtual interface class describing the interface any class needs to implement that should continously
     * process samples from an SDR IO device. processRFSampleBlock will be called repeatedly on a high-priority thread.
     * As real-time-safe application rely on a predictable, fast as possible return time of this call make sure to
     * never perform any system call like memory allocation, console prints, etc from within this callback as those
     * operations might not return after a predictable amount of time depending on the operating system load and other
     * factors. To allocate ressources use prepareForStreaming, which will be guranteed to be called before the first
     * callback to processRFSampleBlock. To free ressources after streaming use the streamingHasStopped callback which
     * will be guranteed to be called after the last call to processRFSampleBlock.
     */
    class SDRIODeviceCallback
    {
    public:
        virtual ~SDRIODeviceCallback() {};

        /**
         * This is the place to set up all your ressources (buffers, DSP blocks, etc.) to get them ready for
         * continously streaming samples.
         */
        virtual void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock) = 0;

        /**
         * This callback will be called repeatedly on a high-priority thread to process the next block of samples. Keep
         * in mind that depending on your setup either the channel count of the input buffer as well as the channel
         * count of the output buffer could be zero. If either the rx or tx path is currently disabled the sample count
         * of the corresponding buffer is guaranteed to be zero.
         */
        virtual void processRFSampleBlock (OptionalCLSampleBufferComplexFloat& rxSamples, OptionalCLSampleBufferComplexFloat& txSamples) = 0;

        /**
         * This callback will be called after the last call to processRFSampleBlock. It is the place to do all your
         * cleanup work.
         */
        virtual void streamingHasStopped() = 0;

        /**
         * If any error occurs during the streaming, this callback will be invoked. Streaming might or might not
         * continue after an error, depending on the type of error.
         */
        virtual void handleError (juce::StringRef errorMessage) = 0;
    };
}