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

#include "MCVReader.h"

namespace ntlab
{
    MCVReader::MCVReader (const juce::File& mcvFile) : file (mcvFile, juce::MemoryMappedFile::readOnly)
    {
        jassert (mcvFile.hasFileExtension ("mcv"));

        if (file.getData() == nullptr)
            return;

        metadata.reset (new MCVHeader (file));

        if (!metadata->isValid())
            return;

        if (file.getSize() != metadata->expectedSizeOfFile())
            return;

        valid = true;

        beginOfSamples = static_cast<void*> (static_cast<char*> (file.getData()) + MCVHeader::sizeOfHeaderInBytes);
    }

    bool MCVReader::isValid () {return valid; }

    bool MCVReader::isComplex () {return metadata->isComplex(); }

    bool MCVReader::hasDoublePrecision () {return metadata->hasDoublePrecision(); }

    int64_t MCVReader::getNumColsOrChannels () {return metadata->getNumColsOrChannels(); }

    int64_t MCVReader::getNumRowsOrSamples () {return metadata->getNumRowsOrSamples(); }

    SampleBufferReal<float> MCVReader::createSampleBufferRealFloat()
    {
        if (isComplex() || (!valid))
            return SampleBufferReal<float> (0, 0);

        SampleBufferReal<float> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillRealFloatBuffer (buf.getArrayOfWritePointers());
        return buf;
    }

    SampleBufferReal<double> MCVReader::createSampleBufferRealDouble()
    {
        if (isComplex() || (!valid))
            return SampleBufferReal<double> (0, 0);

        SampleBufferReal<double> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillRealDoubleBuffer (buf.getArrayOfWritePointers());
        return buf;
    }

    SampleBufferComplex<float> MCVReader::createSampleBufferComplexFloat()
    {
        if (!valid)
            return SampleBufferComplex<float> (0, 0);

        SampleBufferComplex<float> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillComplexFloatBuffer (buf.getArrayOfWritePointers());
        return buf;
    }

    SampleBufferComplex<double> MCVReader::createSampleBufferComplexDouble()
    {
        if (!valid)
            return SampleBufferComplex<double> (0, 0);

        SampleBufferComplex<double> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillComplexDoubleBuffer (buf.getArrayOfWritePointers());
        return buf;
    }

    // Helper macro to fill a buffer with source data that handles all casting if necessary
#define NTLAB_FILL_DESTINATION_BUFFER(sourceDataType, destinationDataType) sourceDataType* sourceBuffer = static_cast<sourceDataType*> (beginOfSamples); \
                                                                           for (int row = 0; row < getNumRowsOrSamples(); ++row)                         \
                                                                           {                                                                             \
                                                                               for (int col = 0; col < getNumColsOrChannels(); ++col)                    \
                                                                               {                                                                         \
                                                                                   destinationBuffer[col][row] = destinationDataType (*sourceBuffer);    \
                                                                                   ++sourceBuffer;                                                       \
                                                                               }                                                                         \
                                                                           }                                                                             \

    void MCVReader::fillRealFloatBuffer (float** destinationBuffer)
    {
        if (hasDoublePrecision())
        {
            NTLAB_FILL_DESTINATION_BUFFER (double, float)
        }
        else
        {
            NTLAB_FILL_DESTINATION_BUFFER (float, float)
        }
    }

    void MCVReader::fillRealDoubleBuffer (double** destinationBuffer)
    {
        if (hasDoublePrecision())
        {
            NTLAB_FILL_DESTINATION_BUFFER (double , double)
        }
        else
        {
            NTLAB_FILL_DESTINATION_BUFFER (float, double)
        }
    }

    void MCVReader::fillComplexFloatBuffer (std::complex<float>** destinationBuffer)
    {
        if (isComplex())
        {
            if (hasDoublePrecision())
            {
                NTLAB_FILL_DESTINATION_BUFFER (std::complex<double>, std::complex<float>)
            }
            else
            {
                NTLAB_FILL_DESTINATION_BUFFER (std::complex<float>, std::complex<float>)
            }
        }
        else
        {
            if (hasDoublePrecision())
            {
                NTLAB_FILL_DESTINATION_BUFFER (double, std::complex<float>)
            }
            else
            {
                NTLAB_FILL_DESTINATION_BUFFER (float, std::complex<float>)
            }

        }
    }

    void MCVReader::fillComplexDoubleBuffer (std::complex<double>** destinationBuffer)
    {
        if (isComplex())
        {
            if (hasDoublePrecision())
            {
                NTLAB_FILL_DESTINATION_BUFFER (std::complex<double>, std::complex<double>)
            }
            else
            {
                NTLAB_FILL_DESTINATION_BUFFER (std::complex<float>, std::complex<double>)
            }
        }
        else
        {
            if (hasDoublePrecision())
            {
                NTLAB_FILL_DESTINATION_BUFFER (double, std::complex<double>)
            }
            else
            {
                NTLAB_FILL_DESTINATION_BUFFER (float, std::complex<double>)
            }

        }
    }
}