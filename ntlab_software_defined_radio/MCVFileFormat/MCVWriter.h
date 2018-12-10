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

#include <juce_core/juce_core.h>
#include "MCVHeader.h"
#include "../SampleBuffers/SampleBuffers.h"

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

        /** Writes a complete SampleBufferReal<float> to the desired output file */
        static bool writeSampleBuffer (SampleBufferReal<float>& bufferToWrite, juce::File& outputFile);

        /** Writes a complete SampleBufferReal<double> to the desired output file */
        static bool writeSampleBuffer (SampleBufferReal<double>& bufferToWrite, juce::File& outputFile);

        /** Writes a complete SampleBufferComplex<float> to the desired output file */
        static bool writeSampleBuffer (SampleBufferComplex<float>& bufferToWrite, juce::File& outputFile);

        /** Writes a complete SampleBufferComplex<double> to the desired output file */
        static bool writeSampleBuffer (SampleBufferComplex<double>& bufferToWrite, juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const float** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const double** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const std::complex<float>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile);

        /** Writes the content of the raw array to the desired output file */
        static bool writeRawArray (const std::complex<double>** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, juce::File& outputFile);

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

        static bool writeRaw (const void** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, bool isComplex, bool isDouble, juce::File& outputFile);
    };
}
