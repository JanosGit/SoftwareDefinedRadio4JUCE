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

#include "juce_core/juce_core.h"
#include "../SampleBuffers/SampleBuffers.h"

namespace ntlab
{
    struct UnitTestHelpers
    {
        template <typename T>
        static void fill1DRawBuffer (T* bufferToFill, int length, juce::Random& random)
        {
            for (int i = 0; i < length; ++i)
                bufferToFill[i] = random.nextInt();
        }

        template <typename T>
        static void fill2DRawBuffer (T** bufferToFill, int dim1, int dim2, juce::Random& random)
        {
            for (int i = 0; i < dim1; ++i)
                fill1DRawBuffer (bufferToFill[i], dim2, random);
        }

        template <typename T>
        static void fillSampleBuffer (T& bufferToFill, juce::Random& random)
        {
            fill2DRawBuffer (bufferToFill.getArrayOfWritePointers(), bufferToFill.getNumChannels(), bufferToFill.getNumSamples(), random);
        }

        template <typename T1, typename T2>
        static bool areEqual1DBuffers (const T1* buffer1, const T2* buffer2, int length)
        {
            for (int i = 0; i < length; ++i)
            {
                if (!juce::approximatelyEqual (buffer1[i], T1(buffer2[i])))
                    return false;
            }

            return true;
        }

        template <typename T1, typename T2>
        static bool areEqual1DBuffers (const std::complex<T1>* buffer1, const std::complex<T2>* buffer2, int length)
        {
            for (int i = 0; i < length; ++i)
            {
                if (!juce::approximatelyEqual (buffer1[i].real(), T1(buffer2[i].real())))
                    return false;

                if (!juce::approximatelyEqual (buffer1[i].imag(), T1(buffer2[i].imag())))
                    return false;
            }

            return true;
        }

        template <typename T1, typename T2>
        static bool areEqual2DBuffers (const T1** buffer1, const T2** buffer2, int dim1, int dim2)
        {
            for (int i = 0; i < dim1; ++i)
            {
                if (!areEqual1DBuffers (buffer1[i], buffer2[i], dim2))
                    return false;
            }

            return true;
        }

        template <typename T1, typename T2>
        static bool areEqualSampleBuffers (T1& buffer1, T2& buffer2)
        {
            if (buffer1.getNumChannels() != buffer2.getNumChannels())
                return false;

            if (buffer1.getNumSamples() != buffer2.getNumSamples())
                return false;

            return areEqual2DBuffers (buffer1.getArrayOfReadPointers(), buffer2.getArrayOfReadPointers(), buffer1.getNumChannels(), buffer1.getNumSamples());
        }

    };
}