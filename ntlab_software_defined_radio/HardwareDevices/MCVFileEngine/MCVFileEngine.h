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
        friend class MCVFileEngineManager;
    public:

        static const juce::Identifier propertyMCVFileEngine;
        static const juce::Identifier propertyInFile;
        static const juce::Identifier propertyOutFile;
        static const juce::Identifier propertyRxEnabled;
        static const juce::Identifier propertyTxEnabled;
        static const juce::Identifier propertyInputEndOfFileBehaviour;
        static const juce::Identifier propertyNumOutChannels;

        /**
         * Sets the file to read from. Note that the file will be closed when streaming has stopped, so if you want to
         * start streaming again you need to reopen it. Depending on the endOfFileBehaviour chosen, stopStreaming will
         * be called automatically when the input file has reached its end. If enbableRx is set to true reading from
         * this file will start as soon as the streaming has been started, otherwise you need to enable it via
         * enableRxTx manually.
         */
        bool setInFile (juce::File& newInFile, MCVReader::EndOfFileBehaviour endOfFileBehaviour = MCVReader::EndOfFileBehaviour::stopAndResize, bool enableRx = true);

        /**
         * Sets the file to write to. Note that if the file already exists, it will be overwritten. If enbableTx is set
         * to true writing to this file will start as soon as the streaming has been started, otherwise you need to
         * enable it via enableRxTx manually.
         */
        bool setOutFile (juce::File& newOutFile, int newNumOutChannels, bool enableTx = true);

        const int getNumRxChannels() override;

        const int getNumTxChannels() override;

        bool setDesiredBlockSize (int newBlockSize) override ;

        bool setSampleRate (double newSampleRate) override;

        double getSampleRate() override;

        juce::ValueTree getDeviceTree() override;

        juce::ValueTree getActiveConfig() override;

        juce::Result setConfig (juce::ValueTree& configToSet) override;

        bool isReadyToStream() override;

        bool startStreaming (SDRIODeviceCallback* callback) override;

        void stopStreaming() override;

        bool isStreaming() override;

        bool enableRxTx (bool enableRx, bool enableTx) override;

    private:

        MCVFileEngine();

        juce::ValueTree engineConfig;

        int blockSize = 512;
        int numOutChannels = 0;
        double sampleRate = 1e6;

        std::unique_ptr<MCVReader> mcvReader;
        std::unique_ptr<MCVWriter> mcvWriter;

        std::unique_ptr<OptionalCLSampleBufferComplexFloat> inSampleBuffer;
        std::unique_ptr<OptionalCLSampleBufferComplexFloat> outSampleBuffer;

        bool rxEnabled = false;
        bool txEnabled = false;

        juce::ThreadPool streamingControlThread;
        SDRIODeviceCallback* activeCallback = nullptr;
        bool streamingIsRunning = false;
        bool shouldStopAtEndOfFile = true;

        bool timerThreadIDisValid = false;
        juce::Thread::ThreadID timerThreadID = nullptr;

        bool timerShouldStopAfterThisCallback = true;
        void hiResTimerCallback() override;

        void reallocateBuffers (bool reallocateInBuffer = true, bool reallocateOutBuffer = true);

        void endStreaming();
    };

    class MCVFileEngineManager : private SDRIOEngineManager
    {
        friend class SDRIOEngineManager;
    private:
        juce::String getEngineName() override;

        juce::Result isEngineAvailable() override;

    protected:
        SDRIOEngine* createEngine() override;
    };

}