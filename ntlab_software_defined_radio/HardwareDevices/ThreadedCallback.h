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

#ifdef NTLAB_FORCED_BLOCKSIZE
namespace ntlab
{

    /** Intended for internal use. Runs the Callback on a separate thread for setups that force a fixed blocksize*/
    class ThreadedCallback : public SDRIODeviceCallback, public juce::Thread
    {
    public:
        ThreadedCallback (SDRIODeviceCallback* callback, bool& rxEn, bool& txEn, int& txBufStartIdx, int numRxChannels, int numTxChannels)
        : juce::Thread     ("FIFORFCallbackThread"),
          originalCallback (*callback),
          rxEnabled        (rxEn),
          txEnabled        (txEn),
          txBufferStartIdx (txBufStartIdx),
#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
          rxSwapBuffer (numRxChannels, NTLAB_FORCED_BLOCKSIZE, SharedCLDevice::getInstance()->getCommandQueue(), SharedCLDevice::getInstance()->getContext(), false, CL_MEM_READ_ONLY,  CL_MAP_WRITE),
          txSwapBuffer (numTxChannels, NTLAB_FORCED_BLOCKSIZE, SharedCLDevice::getInstance()->getCommandQueue(), SharedCLDevice::getInstance()->getContext(), false, CL_MEM_WRITE_ONLY, CL_MAP_READ)
#else
          rxSwapBuffer (numRxChannels, NTLAB_FORCED_BLOCKSIZE),
          txSwapBuffer (numTxChannels, NTLAB_FORCED_BLOCKSIZE)
#endif
        {

        }

        void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock) override
        {
            // 4 times the duration of a sample block seems like a reasonable timeout value
            threadTimeout = static_cast<int> ((NTLAB_FORCED_BLOCKSIZE / sampleRate) * 40000.0);

            startThread (realtimeAudioPriority);
            originalCallback.prepareForStreaming (sampleRate, numActiveChannelsIn, numActiveChannelsOut, NTLAB_FORCED_BLOCKSIZE);
        }

        /**
         * Will simply return if the rx buffer is not filled enough or the start pos of the tx buffer is not at the buffers end.
         * Otherwise it swaps buffers and passes them to the second processing thread
         */
        void processRFSampleBlock (OptionalCLSampleBufferComplexFloat& rxSamples, OptionalCLSampleBufferComplexFloat& txSamples) override
        {
            if (rxEnabled)
            {
                if (rxSamples.getNumSamples() < NTLAB_FORCED_BLOCKSIZE)
                    return;
            }
            else if (txEnabled)
            {
                if (txBufferStartIdx < NTLAB_FORCED_BLOCKSIZE)
                    return;
            }

            processingSyncPoint.wait();
            rxSamples.swapWith (rxSwapBuffer);
            txSamples.swapWith (txSwapBuffer);
            notify();

            rxSamples.setNumSamples (0);
            txBufferStartIdx = 0;
        }

        void streamingHasStopped() override
        {
            stopThread (threadTimeout);
            originalCallback.streamingHasStopped();
        }

        void handleError (const juce::String& errorMessage) override
        {
            originalCallback.handleError (errorMessage);
        }

        void run() override
        {
            processingSyncPoint.signal();

            while (true)
            {
                wait (-1);
                if (threadShouldExit())
                    return;

                originalCallback.processRFSampleBlock (rxSwapBuffer, txSwapBuffer);
                processingSyncPoint.signal();
            }
        }

    private:
        SDRIODeviceCallback& originalCallback;
        bool& rxEnabled;
        bool& txEnabled;
        int&  txBufferStartIdx;

#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
        CLSampleBufferComplex<float> rxSwapBuffer, txSwapBuffer;
#else
        SampleBufferComplex<float> rxSwapBuffer, txSwapBuffer;
#endif
        juce::WaitableEvent processingSyncPoint;
        int threadTimeout = 0;
    };
}

#endif // NTLAB_FORCED_BLOCKSIZE