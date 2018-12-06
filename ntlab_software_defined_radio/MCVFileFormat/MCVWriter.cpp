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
    bool MCVWriter::writeSampleBuffer (ntlab::SampleBufferReal<float>& bufferToWrite, juce::File& outputFile)
    {
        return writeRawArray (bufferToWrite.getArrayOfReadPointers(), bufferToWrite.getNumChannels(), bufferToWrite.getNumSamples(), outputFile);
    }

    bool MCVWriter::writeSampleBuffer (ntlab::SampleBufferReal<double>& bufferToWrite, juce::File& outputFile)
    {
        return writeRawArray (bufferToWrite.getArrayOfReadPointers(), bufferToWrite.getNumChannels(), bufferToWrite.getNumSamples(), outputFile);
    }

    bool MCVWriter::writeSampleBuffer (ntlab::SampleBufferComplex<float>& bufferToWrite, juce::File& outputFile)
    {
        return writeRawArray (bufferToWrite.getArrayOfReadPointers(), bufferToWrite.getNumChannels(), bufferToWrite.getNumSamples(), outputFile);
    }

    bool MCVWriter::writeSampleBuffer (ntlab::SampleBufferComplex<double>& bufferToWrite, juce::File& outputFile)
    {
        return writeRawArray (bufferToWrite.getArrayOfReadPointers(), bufferToWrite.getNumChannels(), bufferToWrite.getNumSamples(), outputFile);
    }

    bool MCVWriter::writeRawArray (const float** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, false, false, outputFile);
    }

    bool MCVWriter::writeRawArray (const double** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, false, true, outputFile);
    }

    bool MCVWriter::writeRawArray (const std::complex<float>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, true, false, outputFile);
    }

    bool MCVWriter::writeRawArray (const std::complex<double>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile)
    {
        return writeRaw (reinterpret_cast<const void**> (rawArray), numColsOrChannels, numRowsOrSamples, true, true, outputFile);
    }

    bool MCVWriter::writeRaw (const void** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, bool isComplex, bool isDouble, juce::File& outputFile)
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