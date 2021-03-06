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

#include "MCVWriter.h"

namespace ntlab
{
    MCVWriter::MCVWriter (int numChannels, bool useDoublePrecision, bool isComplex, const juce::File& outputFile, int fifoSize)
      : juce::AbstractFifo (fifoSize),
        outputStream (outputFile)
    {
        jassert (outputFile.hasFileExtension ("mcv"));
        jassert (numChannels > 0);

        mcvWriterThread->newWriterCreated (this);

        metadata.reset (new MCVHeader (isComplex, useDoublePrecision, numChannels));

        auto numBytesPerFIFOChannel = metadata->sizeOfOneValue() * fifoSize;
        fifoBuffers.reserve (static_cast<size_t> (numChannels));
        tmpChannelPointers.resize (static_cast<size_t> (numChannels));

        for (int i = 0; i < numChannels; ++i)
            fifoBuffers.emplace_back (juce::MemoryBlock (numBytesPerFIFOChannel));

        outputStream.setPosition (MCVHeader::sizeOfHeaderInBytes);
    }

    MCVWriter::~MCVWriter()
    {
        updateMetadataHeader();
        mcvWriterThread->writerDeleted (this);
    }

    bool MCVWriter::isValid () {return outputStream.openedOk(); }

    void MCVWriter::waitForEmptyFIFO (int timeout)
    {
        waitForEmptyFIFOEvent.wait (timeout);
    }

    void MCVWriter::updateMetadataHeader ()
    {
        std::lock_guard<std::mutex> scopedOutFileLock (outputFileLock);

        auto outputStreamPosition = outputStream.getPosition();

        metadata->setNumRowsOrSamples (numSamples);
        metadata->writeToFile (outputStream);

        outputStream.setPosition (outputStreamPosition);
        outputStream.flush();
    }

    bool MCVWriter::writeRawArray (const float** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, false, false, outputFile);
    }

    bool MCVWriter::writeRawArray (const double** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, false, true, outputFile);
    }

    bool MCVWriter::writeRawArray (const std::complex<float>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, true, false, outputFile);
    }

    bool MCVWriter::writeRawArray (const std::complex<double>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, true, true, outputFile);
    }

    MCVWriter::MCVWriterThread::MCVWriterThread () : juce::Thread ("MCVWriter Thread")
    {
        // give it a slightly higher priority than the default one
        startThread (6);
    }

    MCVWriter::MCVWriterThread::~MCVWriterThread ()
    {
        stopThread (100);
    }

    void MCVWriter::MCVWriterThread::newWriterCreated (ntlab::MCVWriter* newWriter)
    {
        writers.add (newWriter);
    }

    void MCVWriter::MCVWriterThread::writerDeleted (ntlab::MCVWriter* writerThatHasBeenDeleted)
    {
        auto idx = writers.indexOf (writerThatHasBeenDeleted);
        writers.remove (idx);
    }

    void MCVWriter::MCVWriterThread::run ()
    {
        while (!threadShouldExit())
        {
            for (auto writer : writers)
            {
                const int numSamplesToWrite = writer->getNumReady();

                if (numSamplesToWrite == 0)
                {
                    writer->waitForEmptyFIFOEvent.signal();
                    continue;
                }


                int fifoStartIdx1, fifoBlockSize1, fifoStartIdx2, fifoBlockSize2;
                writer->prepareToRead (numSamplesToWrite, fifoStartIdx1, fifoBlockSize1, fifoStartIdx2, fifoBlockSize2);

                const auto numChannelsToWrite = writer->metadata->getNumColsOrChannels();
                const auto numBytesPerValue = writer->metadata->sizeOfOneValue();
                auto& outputStream = writer->outputStream;

                std::lock_guard<std::mutex> scopedOutFileLock (writer->outputFileLock);

                if (fifoBlockSize1 > 0)
                {
                    for (int s = 0; s < fifoBlockSize1; ++ s)
                    {
                        for (int c = 0; c < numChannelsToWrite; ++ c)
                        {
                            void* valueToWrite = juce::addBytesToPointer (writer->fifoBuffers[c].getData(), numBytesPerValue * (fifoStartIdx1 +s));
                            outputStream.write (valueToWrite, numBytesPerValue);
                        }
                    }
                    writer->numSamples += fifoBlockSize1;
                }

                if (fifoBlockSize2 > 0)
                {
                    for (int s = 0; s < fifoBlockSize2; ++ s)
                    {
                        for (int c = 0; c < numChannelsToWrite; ++ c)
                        {
                            void* valueToWrite = juce::addBytesToPointer (writer->fifoBuffers[c].getData(), numBytesPerValue * (fifoStartIdx2 +s));
                            outputStream.write (valueToWrite, numBytesPerValue);
                        }
                    }
                    writer->numSamples += fifoBlockSize2;
                }

                writer->finishedRead (fifoBlockSize1 + fifoBlockSize2);

                if (writer->getNumReady() == 0)
                    writer->waitForEmptyFIFOEvent.signal();
            }

            wait (-1);
        }
    }

    void MCVWriter::setTmpChannelPointersToIndex (int index)
    {
        auto numBytesPerValue = metadata->sizeOfOneValue();
        for (int i = 0; i < metadata->getNumColsOrChannels(); ++i)
            tmpChannelPointers[i] = juce::addBytesToPointer (fifoBuffers[i].getData(), index * numBytesPerValue);
    }

    bool MCVWriter::writeRaw (const void** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, bool isComplex, bool isDouble, const juce::File& outputFile)
    {
        jassert (outputFile.hasFileExtension ("mcv"));

        auto header = MCVHeader (isComplex, isDouble, numColsOrChannels, numRowsOrSamples);
        auto numBytesPerValue = header.sizeOfOneValue();

        juce::FileOutputStream outputStream (outputFile);

        if (!outputStream.openedOk())
            return false;

        outputStream.setPosition (0);
        outputStream.truncate();

        if (!header.writeToFile (outputStream))
            return false;

        for (int row = 0; row < numRowsOrSamples; ++row)
        {
            for (int col = 0; col < numColsOrChannels; ++col)
            {
                if (!outputStream.write (static_cast<const char*>(rawArray[col]) + row * numBytesPerValue, numBytesPerValue))
                    return false;
            }
        }

        return true;
    }
}