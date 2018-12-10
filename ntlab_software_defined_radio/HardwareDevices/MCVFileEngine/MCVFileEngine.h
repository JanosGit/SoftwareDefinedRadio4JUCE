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

#include "../SDRIOEngine.h"
#include "../../MCVFileFormat/MCVReader.h"
#include "../../MCVFileFormat/MCVWriter.h"

namespace ntlab
{

    class MCVFileEngine : public SDRIOEngine, private juce::HighResolutionTimer
    {
    public:
        MCVFileEngine();

        bool setInFile (juce::File& newInFile, MCVReader::EndOfFileBehaviour endOfFileBehaviour = MCVReader::EndOfFileBehaviour::stopAndResize);

        bool setOutFile (juce::File& newOutFile, int newNumOutChannels);

        bool setBlockSize (int newBlockSize);

        bool setSampleRate (double newSampleRate);

        juce::String getName() override;

        juce::var getDeviceTree() override;

        juce::var getActiveConfig() override;

        bool setConfig (juce::var& configToSet) override;

        bool isReadyToStream() override;

        bool startStreaming (SDRIODeviceCallback* callback) override ;

        void stopStreaming() override ;

        bool isStreaming() override ;

    private:
        int blockSize = 512;
        int numOutChannels = 0;
        double sampleRate = 1e6;

        std::unique_ptr<MCVReader> mcvReader;
        std::unique_ptr<MCVWriter> mcvWriter;

        std::unique_ptr<OptionalCLSampleBufferComplexFloat> inSampleBuffer;
        std::unique_ptr<OptionalCLSampleBufferComplexFloat> outSampleBuffer;

        juce::ThreadPool streamingControlThread;
        SDRIODeviceCallback* activeCallback = nullptr;
        bool streamingIsRunning = false;
        bool shouldStopAtEndOfFile = true;

        void hiResTimerCallback() override;

        void reallocateBuffers (bool reallocateInBuffer = true, bool reallocateOutBuffer = true);

        void endStreaming();
    };


}