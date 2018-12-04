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
#include <complex>

#if (NTLAB_NO_SIMD == 0)

#ifdef JUCE_INTEL
#define NTLAB_USE_AVX2 1
#elif JUCE_ARM
#define NTLAB_USE_NEON 1
#else
#error "Unsupported target architecture for SIMD operations"
#endif

#endif // NTLAB_NO_SIMD

#if NTLAB_USE_AVX2
#include <immintrin.h>
#endif

namespace ntlab
{
    class SIMDHelpers
    {
        friend struct Allocator;
        friend class ComplexVectorOperations;
    public:

        /**
         * Two functions to allocate and free a vector that is aligned to the requirements of the used SIMD instruction
         * set. If no SIMD is used this will simply invoke new[] and delete[]
         */
        template <typename T>
        struct Allocation
        {
            /**
             * Returns a pointer to a vector with numElements space for elements of type T. Always free it with
             * a call to freeAlignedVector
             */
            static T* allocateAlignedVector (size_t numElements)
            {
#if NTLAB_USE_AVX2
                return static_cast<T*> (_mm_malloc (numElements * sizeof (T), simdRequiredAlignmentBytes));

#elif NTLAB_USE_NEON
                T* alignedMemoryPtr;
                posix_memalign (&alignedMemoryPtr, simdRequiredAlignmentBytes, numElements * sizeof (T));
                return alignedMemoryPtr;
#else
                return new T[numElements];
#endif
            }

            /**
             * Frees a vector that was previously allocated by allocateAlignedVector. Using this to free anything else
             * leads to undefined behaviour.
             */
            static void freeAlignedVector (T* vectorPtr)
            {
#if NTLAB_USE_AVX2
                _mm_free (vectorPtr);
#elif NTLAB_USE_NEON
                free (vectorPtr);
#else
                delete[] (vectorPtr);
#endif
            }
        };

        /** Returns true if the pointer is aligned to the requirements of the used SIMD instruction set */
        static bool isPointerAligned (const void* ptr)
        {
            return (((uintptr_t)ptr) & (simdRequiredAlignmentBytes - 1)) == 0;
        }

        template <typename T>
        struct Partition
        {
            static void forArbitraryLengthVector (const int vectorLength, int& numSIMDVectors, int& numElementsToProcessWithSIMD, int& numElementsRemainingToProcessWithoutSIMD);
        };


    private:
#if NTLAB_USE_AVX2
        static const int simdRequiredAlignmentBytes = 32;
        static const int simdVectorLengthDouble = 4;
        static const int simdVectorLengthFloat = 8;
        static const int simdVectorLengthInt32 = 8;
        static const int simdVectorLengthInt16 = 16;
#elif NTLAB_USE_NEON
        static const int simdRequiredAlignmentBytes = 16;
#else
        static const int simdRequiredAlignmentBytes = 1;
        static const int simdVectorLengthDouble = 1;
        static const int simdVectorLengthFloat = 1;
        static const int simdVectorLengthInt32 = 1;
        static const int simdVectorLengthInt16 = 1;
#endif
    };


    class ComplexVectorOperations
    {
    public:

        /** Copies all real values from the complex valued input vector to the real output vector */
        static void extractRealPart (const std::complex<float>* complexInVector, float* realOutVector, int length);

        /** Copies all imaginary values from the complex valued input vector to the imaginary output vector */
        static void extractImagPart (const std::complex<float>* complexInVector, float* imagOutVector, int length);

        /**
         * Copies all real values from the complex valued input vector to the real output vector and all imaginary
         * values to the imaginary output vector. This is slightly faster than executing extractRealPart and
         * extractImagPart successively if you need both parts.
         */
        static void extractRealAndImagPart (const std::complex<float>* complexInVector, float* realOutVector, float* imagOutVector, int length);

        /** Calculates the absolute values of the complex vector */
        static void abs (const std::complex<float>* complexInVector, float* absOutVector, int length);

    private:

        int calculateNumElementsToProcessWithoutSIMD (int numElementsToProcess, int vectorLength)
        {
            return numElementsToProcess - ((numElementsToProcess / vectorLength) * vectorLength);
        }


        template <typename T>
        static void extractRealPartNonSIMD (const std::complex<T>* complexInVector, T* realOutVector, int length)
        {
            for (int i = 0; i < length; ++i)
                realOutVector[i] = complexInVector[i].real();
        }

        template <typename T>
        static void extractImagPartNonSIMD (const std::complex<T>* complexInVector, T* imagOutVector, int length)
        {
            for (int i = 0; i < length; ++i)
                imagOutVector[i] = complexInVector[i].imag();
        }

        template <typename T>
        static void extractRealAndImagPartNonSIMD (const std::complex<T>* complexInVector, T* realOutVector, T* imagOutVector, int length)
        {
            for (int i = 0; i < length; ++i)
            {
                realOutVector[i] = complexInVector[i].real();
                imagOutVector[i] = complexInVector[i].imag();
            }
        }

        template <typename T>
        static void absNonSIMD (const std::complex<T>* complexInVector, T* absOutVector, int length)
        {
            for (int i = 0; i < length; ++i)
                absOutVector[i] = std::abs (complexInVector[i]);
        }
    };
}