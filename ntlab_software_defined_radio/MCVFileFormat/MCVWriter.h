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
    class MCVWriter
    {
    public:


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

        static bool writeRaw (const void** rawArray, int64_t numColsOrChannels, int64_t numRowsOrSamples, bool isComplex, bool isDouble, juce::File& outputFile);

    };
}
