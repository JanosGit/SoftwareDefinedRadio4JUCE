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

#include "MCVFileEngine.h"
#include "../../UnitTestHelpers/UnitTestHelpers.h"

namespace ntlab
{
    MCVFileEngine::MCVFileEngine () : streamingControlThread (1) {}

    bool MCVFileEngine::setInFile (juce::File& newInFile, ntlab::MCVReader::EndOfFileBehaviour endOfFileBehaviour)
    {
        if (streamingIsRunning)
            return false;

        if (! (newInFile.existsAsFile() && newInFile.hasFileExtension ("mcv")))
            return false;

        mcvReader.reset (new MCVReader (newInFile, endOfFileBehaviour));

        if (!mcvReader->isValid())
        {
            mcvReader.reset (nullptr);
            return false;
        }

        shouldStopAtEndOfFile = endOfFileBehaviour != MCVReader::EndOfFileBehaviour::loop;

        reallocateBuffers (true, false);

        return true;
    }

    bool MCVFileEngine::setOutFile (juce::File& newOutFile, int newNumOutChannels)
    {
        if (streamingIsRunning)
            return false;

        if (! newOutFile.hasFileExtension ("mcv"))
            return false;

        mcvWriter.reset (new MCVWriter (newNumOutChannels, false, true, newOutFile));

        if (!mcvWriter->isValid())
        {
            mcvWriter.reset (nullptr);
            return false;
        }

        numOutChannels = newNumOutChannels;
        reallocateBuffers (false, true);

        return true;
    }

    bool MCVFileEngine::setBlockSize (int newBlockSize)
    {
        if (streamingIsRunning)
            return false;

        blockSize = newBlockSize;
        reallocateBuffers();

        return true;
    }

    bool MCVFileEngine::setSampleRate (double newSampleRate)
    {
        if (isStreaming())
            return false;

        sampleRate = newSampleRate;
        return true;
    }

    juce::String MCVFileEngine::getName() {return "MCV File Engine"; }

    juce::var MCVFileEngine::getDeviceTree()
    {
        return juce::var();
    }

    juce::var MCVFileEngine::getActiveConfig()
    {
        return juce::var();
    }

    bool MCVFileEngine::setConfig (juce::var& configToSet)
    {
        return juce::var();
    }

    bool MCVFileEngine::isReadyToStream()
    {
        return (inSampleBuffer!= nullptr) || (outSampleBuffer != nullptr);
    }

    bool MCVFileEngine::startStreaming (ntlab::SDRIODeviceCallback* callback)
    {
        if (!isReadyToStream())
            return false;

        if (streamingIsRunning)
        {
            if (callback == activeCallback)
                return true;

            stopStreaming();
        }

        streamingIsRunning = true;
        activeCallback = callback;
        auto setUpStreaming = [this]()
        {
            int numInChannels = 0;
            if (mcvReader != nullptr)
                numInChannels = static_cast<int> (mcvReader->getNumColsOrChannels());

            activeCallback->prepareForStreaming (sampleRate, numInChannels, numOutChannels);

            double timerIntervalInSeconds = blockSize / sampleRate;

            startTimer (juce::roundToInt (timerIntervalInSeconds / 1000.0));
        };

        streamingControlThread.addJob (setUpStreaming);

        return true;
    }

    void MCVFileEngine::stopStreaming()
    {
        if (!streamingIsRunning)
            return;

        streamingControlThread.addJob ([this]() {endStreaming(); });
    }

    bool MCVFileEngine::isStreaming() {return streamingIsRunning; }

    void MCVFileEngine::hiResTimerCallback ()
    {
        bool shouldStopAfterThisCallback = false;

        if (mcvReader != nullptr)
        {
            shouldStopAfterThisCallback = (!mcvReader->fillNextSamplesIntoBuffer (*inSampleBuffer)) && shouldStopAtEndOfFile;

            if (mcvWriter != nullptr)
                outSampleBuffer->setNumSamples (inSampleBuffer->getNumSamples());
        }

        activeCallback->processRFSampleBlock (*inSampleBuffer, *outSampleBuffer);

        if (mcvWriter != nullptr)
        {
            mcvWriter->appendSampleBuffer (*outSampleBuffer);
        }

        outSampleBuffer->setNumSamples (outSampleBuffer->getMaxNumSamples());
        inSampleBuffer ->setNumSamples (inSampleBuffer ->getMaxNumSamples());

        if (shouldStopAfterThisCallback)
            endStreaming();
    }

    void MCVFileEngine::reallocateBuffers (bool reallocateInBuffer, bool reallocateOutBuffer)
    {
         if ((mcvReader != nullptr) && reallocateInBuffer)
         {
             inSampleBuffer.reset (new OptionalCLSampleBufferComplexFloat (static_cast<int> (mcvReader->getNumColsOrChannels()), blockSize));
         }
         else if (mcvReader == nullptr)
         {
             inSampleBuffer.reset (new OptionalCLSampleBufferComplexFloat (0, 0));
         }

         if ((mcvWriter != nullptr) && reallocateOutBuffer)
         {
             outSampleBuffer.reset (new OptionalCLSampleBufferComplexFloat (numOutChannels, blockSize));
         }
         else if (mcvWriter == nullptr)
         {
             outSampleBuffer.reset (new OptionalCLSampleBufferComplexFloat (0, 0));
         }
    }

    void MCVFileEngine::endStreaming()
    {
        stopTimer();
        mcvWriter->waitForEmptyFIFO();
        mcvWriter->updateMetadataHeader();
        streamingIsRunning = false;

        activeCallback->streamingHasStopped();

        activeCallback = nullptr;
    }

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS

    class MCVFileEngineTest : public juce::UnitTest, private SDRIODeviceCallback
    {
    public:
        MCVFileEngineTest() : juce::UnitTest ("MCVFileEngine test") {};

        void runTest() override
        {
            auto tempFolder = juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory);
            auto inFile  = tempFolder.getChildFile ("inFile.mcv");
            auto outFile = tempFolder.getChildFile ("outFile.mcv");

            auto random = getRandom();

            SampleBufferComplex<float>  srcBuffer  (numChannels, numSamples);
            UnitTestHelpers::fillSampleBuffer (srcBuffer, random);
            MCVWriter::writeSampleBuffer (srcBuffer, inFile);

            beginTest ("Create MCVFileEngine instance");

            MCVFileEngine mcvFileEngine;
            expect (mcvFileEngine.setInFile (inFile));
            expect (mcvFileEngine.setOutFile (outFile, numChannels));
            expect (mcvFileEngine.setSampleRate (sampleRate));

            mcvFileEngine.startStreaming (this);

            waitForEngineToFinish.wait (-1);

            beginTest ("Comparing in and out file");

            MCVReader mcvReader (outFile, MCVReader::EndOfFileBehaviour::stopAndResize);
            expect (mcvReader.isValid ());
            auto sinkBuffer = mcvReader.createSampleBufferComplexFloat();
            expect (UnitTestHelpers::areEqualSampleBuffers (srcBuffer, sinkBuffer));

            inFile.deleteFile();
            outFile.deleteFile();
        }

    private:
        static const int numChannels = 4;
        static const int numSamples = 3000;
        static const int sampleRate = 2000000;

        juce::WaitableEvent waitForEngineToFinish;

        void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut) override
        {
            beginTest ("prepareForStreaming callback");

            expect (sampleRate == this->sampleRate);
            expect (numActiveChannelsIn  == numChannels);
            expect (numActiveChannelsOut == numChannels);
        }

        void processRFSampleBlock (OptionalCLSampleBufferComplexFloat& inSamples, OptionalCLSampleBufferComplexFloat& outSamples) override
        {
            auto numInSamples = inSamples.getNumSamples();
            auto numChannels = inSamples.getNumChannels();
            inSamples.copyTo (outSamples, numInSamples, numChannels);
        }

        void streamingHasStopped() override
        {
            beginTest ("streamingHasStopped callback");

            waitForEngineToFinish.signal();
        }

        void handleError (const juce::String& errorMessage) override
        {
            beginTest ("handleError callback");

            // this callback should not be called
            expect (false, errorMessage);
        }

    };

    static MCVFileEngineTest mcvFileEngineTest;

#endif // NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
}