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
         * Describes the behaviour the reader shows when the end of the File is reached when interacting with
         * sample buffers
         */
        enum EndOfFileBehaviour
        {
            /**
             * If a buffer is handed to the reader that has a bigger capacity than the number of samples the reader can
             * supply left, this buffer is filled with the remaining samples the reader can supply. The rest of the
             * buffer will be filled with zeros. The readers read position will stop at the end of the file. Successive
             * buffers will be filled with zeros only.
             */
            stopAndFillWithZeros,

            /**
             * If a buffer is handed to the reader that has a bigger capacity than the number of samples the reader can
             * supply left, this buffer is filled with the remaining samples the reader can supply. The buffers number
             * of samples will be set to the number of samples filled into the buffer. The readers read position will
             * stop at the end of the file. Successive buffers will be resized to a size of zero samples.
             */
            stopAndResize,

            /**
             * The reader will just wrap over to the beginning of the file when reaching the end. This means if a buffer
             * is handed to the reader that has a bigger capacity than the number of samples the reader can supply left,
             * this buffer is first filled with the remaining samples the reader can supply and then filled with samples
             * from the beginning of the file.
             */
            loop
        };

        /**
         * Creates an MCVReader object from an mcv file. Always call isValid after creation to check if opening the
         * file was successful.
         */
        MCVReader (const juce::File& mcvFile, EndOfFileBehaviour endOfFileBehaviour = stopAndFillWithZeros);

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

        /**
         * Fills a buffer with the next samples from the file. You have to make sure that the buffer passed matches in
         * channel count as well as in the data type. This means: If the file content is complexed valued and the buffer
         * passed is real valued behaviour is undefined. After having filled the buffer, the files read position will
         * be advanced to the next sample after the block just read. The behaviour in case the end of file is reached
         * before the whole buffer could be filled is determined by the endOfFileBehaviour flag passed to the
         * MCVReader constructor
         */
        template <typename BufferType>
        void fillNextSamplesIntoBuffer (BufferType& bufferToFill, int64_t startSampleInBuffer = 0)
        {
            // Always make sure the buffer you want to fill matches the channel count
            jassert (bufferToFill.getNumChannels() == metadata->getNumColsOrChannels());

            int64_t bufferSampleCapacity = bufferToFill.getNumSamples() - startSampleInBuffer;
            int64_t numSamplesToCopy = std::min (bufferSampleCapacity, numRowsOrSamplesRemaining);

            bufferToFill.setNumSamples (numSamplesToCopy);
            fillBuffer (bufferToFill.getArrayOfWritePointers(), readPtr, numSamplesToCopy, startSampleInBuffer);
            numRowsOrSamplesRemaining -= numSamplesToCopy;
            readPtr = readPositionPtrForSample (getReadPosition());

            if (numSamplesToCopy < bufferSampleCapacity)
            {
                switch (behaviour)
                {
                    case stopAndResize:
                        break;
                    case stopAndFillWithZeros:
                        bufferToFill.setNumSamples (bufferSampleCapacity);
                        bufferToFill.clearBufferRegion (numSamplesToCopy);
                        break;
                    case loop:
                        readPtr = beginOfSamples;
                        numRowsOrSamplesRemaining = metadata->getNumRowsOrSamples();
                        fillNextSamplesIntoBuffer (bufferToFill, numSamplesToCopy);
                        break;
                }
            }
        }

        /** Returns the row or sample that gets read with the next call of fillNextSamplesIntoBuffer */
        int64_t getReadPosition();

    private:
        juce::MemoryMappedFile file;
        std::unique_ptr<MCVHeader> metadata;

        void* beginOfSamples;
        void* readPtr;
        int64_t numRowsOrSamplesRemaining;
        EndOfFileBehaviour behaviour;

        bool valid = false;

        void fillBuffer (float** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample = 0);

        void fillBuffer (double** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample = 0);

        void fillBuffer (std::complex<float>** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample = 0);

        void fillBuffer (std::complex<double>** destinationBuffer, void* sourceStart, int64_t numRowsOrSamples, int64_t destinationStartRowOrSample = 0);

        void* readPositionPtrForSample (int64_t sampleIdx);
    };
}