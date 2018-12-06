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

#include "UnitTestHelpers.h"

namespace ntlab
{
    juce::Range<int> int16Range (std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());

    template <>
    void UnitTestHelpers::fill1DRawBuffer<float> (float* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = random.nextFloat();
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<double> (double* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = random.nextDouble();
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<int16_t> (int16_t* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = static_cast<int16_t> (random.nextInt (int16Range));
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<std::complex<float>> (std::complex<float>* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = std::complex<float> (random.nextFloat(), random.nextFloat());
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<std::complex<double>> (std::complex<double>* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = std::complex<double> (random.nextDouble(), random.nextDouble());
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<std::complex<int>> (std::complex<int>* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = std::complex<int> (random.nextInt(), random.nextInt());
    }

    template <>
    void UnitTestHelpers::fill1DRawBuffer<std::complex<int16_t>> (std::complex<int16_t>* bufferToFill, int length, juce::Random& random)
    {
        for (int i = 0; i < length; ++i)
            bufferToFill[i] = std::complex<int16_t> (static_cast<int16_t> (random.nextInt (int16Range)), static_cast<int16_t> (random.nextInt (int16Range)));
    }
}