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
    /**
     * A class to read MCV files. Contains some methods to create a buffer from the whole file content at once as well
     * as the possibility to dynamically read from an MCV file. For more info on the MCV format refer to the Readme file
     * in the MCVFileFormat folder in the root folder of this repo.
     */
    class MCVReader
    {
    public:

        /**
         * Creates an MCVReader object from an mcv file. Always call isValid after creation to check if opening the
         * file was successful.
         */
        MCVReader (const juce::File& mcvFile);

        /** Returns true if opening the file was successful */
        bool isValid();

        /** Returns true if the file contains complex values, false if real values */
        bool isComplex();

        /** Returns true if the values in the file are doubles, false if floats */
        bool hasDoublePrecision();

        /** Returns the number of columns / channels held by this file */
        int64_t getNumColsOrChannels();

        /** Returns the number of rows / samples held by this file */
        int64_t getNumRowsOrSamples();

        /**
         * Tries to create a SampleBufferReal<float> from the whole mcv file content. This will succeed if the file
         * contains real values, if the file contains complex values or if the file has not been opened successful
         * it will return an empty buffer.
         */
        SampleBufferReal<float> createSampleBufferRealFloat();

        /**
         * Tries to create a SampleBufferReal<double> from the whole mcv file content. This will succeed if the file
         * contains real values, if the file contains complex values or if the file has not been opened successful
         * it will return an empty buffer.
         */
        SampleBufferReal<double> createSampleBufferRealDouble();

        /**
         * Tries to create a SampleBufferComplex<float> from the whole mcv file content. If the file contains real
         * values all imaginary values in the resulting file will be set to 0. It the file has not been opened
         * successful it will return an empty buffer.
         */
        SampleBufferComplex<float> createSampleBufferComplexFloat();

        /**
         * Tries to create a SampleBufferComplex<float> from the whole mcv file content. If the file contains real
         * values all imaginary values in the resulting file will be set to 0. It the file has not been opened
         * successful it will return an empty buffer.
         */
        SampleBufferComplex<double> createSampleBufferComplexDouble();

    private:
        juce::MemoryMappedFile file;
        std::unique_ptr<MCVHeader> metadata;

        void* beginOfSamples;
        int readPosition = 0;

        bool valid = false;

        void fillRealFloatBuffer (float** destinationBuffer);

        void fillRealDoubleBuffer (double** destinationBuffer);

        void fillComplexFloatBuffer (std::complex<float>** destinationBuffer);

        void fillComplexDoubleBuffer (std::complex<double>** destinationBuffer);
    };
}