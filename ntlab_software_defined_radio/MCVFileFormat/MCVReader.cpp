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
    MCVReader::MCVReader (const juce::File& mcvFile, EndOfFileBehaviour endOfFileBehaviour)
      : file (mcvFile, juce::MemoryMappedFile::readOnly, true),
        behaviour (endOfFileBehaviour)
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
        beginOfSamples = juce::addBytesToPointer (file.getData(), MCVHeader::sizeOfHeaderInBytes);
        readPtr = beginOfSamples;
        numRowsOrSamplesRemaining = metadata->getNumRowsOrSamples();
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
        fillBuffer (buf.getArrayOfWritePointers(), beginOfSamples, getNumRowsOrSamples ());
        return buf;
    }

    SampleBufferReal<double> MCVReader::createSampleBufferRealDouble()
    {
        if (isComplex() || (!valid))
            return SampleBufferReal<double> (0, 0);

        SampleBufferReal<double> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillBuffer (buf.getArrayOfWritePointers(), beginOfSamples, getNumRowsOrSamples());
        return buf;
    }

    SampleBufferComplex<float> MCVReader::createSampleBufferComplexFloat()
    {
        if (!valid)
            return SampleBufferComplex<float> (0, 0);

        SampleBufferComplex<float> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillBuffer (buf.getArrayOfWritePointers(), beginOfSamples, getNumRowsOrSamples());
        return buf;
    }

    SampleBufferComplex<double> MCVReader::createSampleBufferComplexDouble()
    {
        if (!valid)
            return SampleBufferComplex<double> (0, 0);

        SampleBufferComplex<double> buf (static_cast<int> (getNumColsOrChannels()), static_cast<int> (getNumRowsOrSamples()));
        fillBuffer (buf.getArrayOfWritePointers(), beginOfSamples, getNumRowsOrSamples());
        return buf;
    }

#if NTLAB_INCLUDE_EIGEN
    Eigen::MatrixXf MCVReader::createMatrixRealFloat()
    {
        if (isComplex() || (!valid))
            return Eigen::MatrixXf();

        if (hasDoublePrecision())
            return createMatrixRealDouble().cast<float>();

        return Eigen::Map<Eigen::MatrixXf> (static_cast<float*> (beginOfSamples), getNumRowsOrSamples(), getNumColsOrChannels());
    }

    Eigen::MatrixXd MCVReader::createMatrixRealDouble()
    {
        if (isComplex() || (!valid))
            return Eigen::MatrixXd();

        if (!hasDoublePrecision())
            return createMatrixRealFloat().cast<double>();

        return Eigen::Map<Eigen::MatrixXd> (static_cast<double*> (beginOfSamples), getNumRowsOrSamples(), getNumColsOrChannels());
    }

    Eigen::MatrixXcf MCVReader::createMatrixComplexFloat()
    {
        if (!valid)
            return Eigen::MatrixXcf();

        if (isComplex())
        {
            if (hasDoublePrecision())
                return createMatrixComplexDouble().cast<std::complex<float>>();

            return Eigen::Map<Eigen::MatrixXcf> (static_cast<std::complex<float>*> (beginOfSamples), getNumRowsOrSamples(), getNumColsOrChannels());
        }

        if (hasDoublePrecision())
            return createMatrixRealDouble().cast<std::complex<float>>();

        return createMatrixRealDouble().cast<std::complex<float>>();
    }

    Eigen::MatrixXcd MCVReader::createMatrixComplexDouble ()
    {
        if (!valid)
            return Eigen::MatrixXcd();

        if (isComplex())
        {
            if (hasDoublePrecision())
                return Eigen::Map<Eigen::MatrixXcd> (static_cast<std::complex<double>*> (beginOfSamples), getNumRowsOrSamples(), getNumColsOrChannels());

            return createMatrixComplexFloat().cast<std::complex<double>>();
        }

        if (hasDoublePrecision())
            return createMatrixRealDouble().cast<std::complex<double>>();

        return createMatrixRealFloat().cast<std::complex<double>>();
    }
#endif

    int64_t MCVReader::getReadPosition() {return metadata->getNumRowsOrSamples() - numRowsOrSamplesRemaining; }

    // Helper macro to fill a buffer with source data that handles all casting if necessary
#define NTLAB_FILL_DESTINATION_BUFFER(sourceDataType, destinationDataType) sourceDataType* sourceBuffer = static_cast<sourceDataType*> (sourceStart);     \
                                                                           for (int64_t row = destinationStartRowOrSample; row < numRowsOrSamples; ++row) \
                                                                           {                                                                              \
                                                                               for (int64_t col = 0; col < getNumColsOrChannels(); ++col)                 \
                                                                               {                                                                          \
                                                                                   destinationBuffer[col][row] = destinationDataType (*sourceBuffer);     \
                                                                                   ++sourceBuffer;                                                        \
                                                                               }                                                                          \
                                                                           }                                                                              \

    void MCVReader::fillBuffer (float** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample)
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

    void MCVReader::fillBuffer (double** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample)
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

    void MCVReader::fillBuffer (std::complex<float>** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample)
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

    void MCVReader::fillBuffer (std::complex<double>** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample)
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

    void* MCVReader::readPositionPtrForSample (int64_t sampleIdx)
    {
        return juce::addBytesToPointer (beginOfSamples, metadata->sizeOfOneValue() * sampleIdx * getNumColsOrChannels());
    }
}