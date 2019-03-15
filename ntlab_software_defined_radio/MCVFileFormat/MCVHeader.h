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
#include <bitset>

namespace ntlab
{
    /** A helper class to read and write MCV files */
    class MCVHeader
    {
    public:
        /** Creates a valid header that can be written to a file */
        MCVHeader (bool isComplex, bool hasDoublePrecision, int64_t numColsOrChannels, int64_t numRowsOrSamples = 0)
        {
            std::strncpy (identifier, "NTLABMC", 7);
            flags.set (0, isComplex);
            flags.set (1, hasDoublePrecision);
            this->numColsOrChannels = numColsOrChannels;
            this->numRowsOrSamples  = numRowsOrSamples;
        }

        /** Loads the header from a FileInputStream */
        MCVHeader (juce::FileInputStream& fileInputStream)
        {
            auto previousPosition = fileInputStream.getPosition();
            if (fileInputStream.getNumBytesRemaining() >= sizeOfHeaderInBytes)
            {
                fileInputStream.read (identifier, 7);
                fileInputStream.read (&flags, 1);
                fileInputStream.read (&numColsOrChannels, 8);
                fileInputStream.read (&numRowsOrSamples,  8);
            }
            else
            {
                // if you got here you tried to load the header from a file not containing enough bytes to even hold the
                // MCV Header.
                jassertfalse;

                // make sure the identifier is invalid
                identifier[0] = 'i';
            }

            fileInputStream.setPosition (previousPosition);
        }

        /** Loads the header from a MemoryMappedFile */
        MCVHeader (juce::MemoryMappedFile& memoryMappedFile)
        {
            auto data = static_cast<const char*> (const_cast<const void*> (memoryMappedFile.getData()));
            // you are trying to read from an invalid memory mapped file
            jassert (data != nullptr);

            if (memoryMappedFile.getSize() >= sizeOfHeaderInBytes)
            {
                std::memcpy (identifier,         data,      7);
                std::memcpy (&flags,             data + 7,  1);
                std::memcpy (&numColsOrChannels, data + 8,  8);
                std::memcpy (&numRowsOrSamples,  data + 16, 8);
            }
            else
            {
                // if you got here you tried to load the header from a file not containing enough bytes to even hold the
                // MCV Header.
                jassertfalse;

                // make sure the identifier is invalid
                identifier[0] = 'i';
            }
        }

        /** Creates an invalid MCV header object */
        static MCVHeader invalid()
        {
            return MCVHeader();
        }

        /** Writes the header to the beginning of a file */
        bool writeToFile (juce::FileOutputStream& fileOutputStream)
        {
            fileOutputStream.setPosition (0);
            if (!fileOutputStream.write (identifier, 7))
                return false;
            if (!fileOutputStream.write (&flags, 1))
                return false;
            if (!fileOutputStream.write (&numColsOrChannels, 8))
                return false;
            if (!fileOutputStream.write (&numRowsOrSamples, 8))
                return false;

            return true;
        }

        /** Cheks if this header is a valid MCV file header */
        bool isValid() {return std::strncmp (identifier, "NTLABMC", 7) == 0; }

        /** Returns true if the MCV file contains complex values */
        bool isComplex() {return flags[0]; }

        /** Returns true if the MCV file contains double precision values */
        bool hasDoublePrecision() {return flags[1]; }

        size_t sizeOfOneValue()
        {
            size_t size = 4;
            if (isComplex())
                size *= 2;

            if (hasDoublePrecision())
                size *= 2;

            return size;
        }

        size_t expectedSizeOfFile()
        {
            size_t size = sizeOfHeaderInBytes;
            size += sizeOfOneValue() * numColsOrChannels * numRowsOrSamples;
            return size;
        }

        void setNumColsOrChannels (int64_t newNumColsOrChannels)
        {
            jassert (newNumColsOrChannels > 0);
            numColsOrChannels = newNumColsOrChannels;
        }

        void setNumRowsOrSamples (int64_t newNumRowsOrSamples)
        {
            jassert (newNumRowsOrSamples > 0);
            numRowsOrSamples = newNumRowsOrSamples;
        }

        /** Returns the number of columns or channels contained in this MCV file */
        int64_t getNumColsOrChannels() {return numColsOrChannels; }

        /** Returns the number of rows or samples contained in this MCV file */
        int64_t getNumRowsOrSamples() {return numRowsOrSamples; }

        /** A constant used by some classes internally */
        static const int sizeOfHeaderInBytes = 24;

    private:
        char identifier[7];
        // bit 0: complex, bit 1: double, other bits: unused
        std::bitset<8> flags;
        int64_t numColsOrChannels;
        int64_t numRowsOrSamples;

        // invalid header
        MCVHeader() {identifier[0] = 0; }
    };
}