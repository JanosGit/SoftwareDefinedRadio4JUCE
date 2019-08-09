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

#if NTLAB_INCLUDE_EIGEN
#include "../Matrix/Eigen/Eigen/Dense"
#endif

#include <juce_core/juce_core.h>
#include "MCVHeader.h"
#include "../SampleBuffers/SampleBuffers.h"
#include <mutex>

namespace ntlab
{
    /**
     * A class to write MCV files. Contains some static methods to write a whole buffer at once as well as the
     * possibility to dynamically write to an MCV file. For more info on the MCV format refer to the Readme file in the
     * MCVFileFormat folder in the root folder of this repo.
     */
    class MCVWriter : private juce::AbstractFifo
    {
        friend class MCVWriterThread;
    public:

        /**
         * Create an MCVWriter instance if you want to continously write and append samples to a MCV File. In case you
         * want to write a matrix or a sample buffer at once, use on of the static member functions
         */
        MCVWriter (int numChannels, bool useDoublePrecision, bool isComplex, juce::File& outputFile, int fifoSize = 8192);

        ~MCVWriter();

        /** Returns true if the writer could successfully open the output file and is ready to write */
        bool isValid();

        /**
         * This updates the metadata header at the beginning of the file so that it stays valid. This is done
         * automatically in the destructor, however calling this in between makes keeps the file valid before
         * deleting the writer.
         */
        void updateMetadataHeader();

        /**
         * As all writes through the appendSampleBuffer functions are written to a FIFO first and then actually written
         * to disk by a dedicated writer thread. Calling this function will lead to wait for the thread to finalize all
         * disk writing until the fifo is empty.
         */
        void waitForEmptyFIFO (int timeout = -1);

        /** Appends the content of this sample buffer to the file */
        void appendSampleBuffer (SampleBufferReal<float>& bufferToAppend);

        /** Appends the content of this sample buffer to the file */
        void appendSampleBuffer (SampleBufferReal<double>& bufferToAppend);

        /** Appends the content of this sample buffer to the file */
        void appendSampleBuffer (SampleBufferComplex<float>& bufferToAppend);

        /** Appends the content of this sample buffer to the file */
        void appendSampleBuffer (SampleBufferComplex<double>& bufferToAppend);

        /** Writes a SampleBuffer to the desired output file. Allowed types are all ntlab SampleBuffer classes and juce
         * AudioBuffer. In case you pass in an ntlab CLSampleBuffer, make sure it is mapped.
         */
        template <typename BufferType>
        static bool writeSampleBuffer (const BufferType& bufferToWrite, const juce::File& outputFile)
        {
            // Make sure your CL buffer is mapped
            if constexpr (IsSampleBuffer<BufferType>::cl())
                jassert (bufferToWrite.isCurrentlyMapped());

            return writeRawArray (bufferToWrite.getArrayOfReadPointers(), bufferToWrite.getNumChannels(), bufferToWrite.getNumSamples(), outputFile);
        };

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const float** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const double** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const std::complex<float>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const std::complex<double>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, const juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file. This version writes a 1D array to a row vector */
        template <typename ArrayType>
        static bool writeRawArray (const ArrayType* rawArray, int64_t numElements, juce::File& outputFile) { return writeRawArray (&rawArray, 1, numElements, outputFile); }

#if NTLAB_INCLUDE_EIGEN
        /** Writes the content of an Eigen::Matrix to the output file. Only available if NTLAB_INCLUDE_EIGEN is enabled */
        template <typename T, int Rows, int Cols>
        static bool writeEigenMatrix (Eigen::Matrix<T, Rows, Cols>& matrixToWrite, juce::File& outputFile)
        {
            static_assert ((std::is_same<T, float>::value ||
                           std::is_same<T, double>::value ||
                           std::is_same<T, std::complex<float>>::value ||
                           std::is_same<T, std::complex<double>>::value),
                           "Only real and complex float and double matrices are supported by ntlab::MCVWriter::writeEigenMatrix");

            jassert (outputFile.hasFileExtension ("mcv"));

            const auto numCols = matrixToWrite.cols();
            const auto numRows = matrixToWrite.rows();

            if (numCols == 0)
                return false;

            auto header = MCVHeader::invalid();

            if (std::is_same<T, float>::value)
                header = MCVHeader (false, false, numCols, numRows);
            else if (std::is_same<T, double>::value)
                header = MCVHeader (false, true, numCols, numRows);
            else if (std::is_same<T, std::complex<float>>::value)
                header = MCVHeader (true, false, numCols, numRows);
            else if (std::is_same<T, std::complex<double>>::value)
                header = MCVHeader (true, true, numCols, numRows);

            juce::FileOutputStream outputStream (outputFile);
            if (!outputStream.openedOk())
                return false;

            outputStream.setPosition (0);
            outputStream.truncate();

            if (!header.writeToFile (outputStream))
                return false;

            return outputStream.write (matrixToWrite.data(), numRows * numCols * header.sizeOfOneValue());
        }
#endif

    private:

        class MCVWriterThread : public juce::Thread
        {
        public:
            MCVWriterThread();
            ~MCVWriterThread();

            void newWriterCreated (MCVWriter* newWriter);
            void writerDeleted (MCVWriter* writerThatHasBeenDeleted);

            void run() override;
        private:
            juce::Array<MCVWriter*, juce::SpinLock> writers;
        };

        juce::SharedResourcePointer<MCVWriterThread> mcvWriterThread;
        juce::WaitableEvent waitForEmptyFIFOEvent;

        std::vector<juce::MemoryBlock> fifoBuffers;
        std::vector<void*> tmpChannelPointers;

        juce::FileOutputStream outputStream;
        std::mutex outputFileLock;
        std::unique_ptr<MCVHeader> metadata;
        int numSamples = 0;

        void setTmpChannelPointersToIndex (int index);

        static bool writeRaw (const void** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, bool isComplex, bool isDouble, const juce::File& outputFile);
    };
}
