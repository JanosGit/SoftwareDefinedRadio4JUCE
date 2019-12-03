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
#include "Eigen/Eigen/Dense"
#endif

#include "../SampleBuffers/SampleBuffers.h"
#include <numeric>


// !!!!!!!! Work in Progress. Nicht fertig, nur als Ansatz zu sehen
namespace ntlab
{
    /**
     * Computes a covariance matrix over definable number of complex valued samples from a continous input stream.
     * It will invoke the callback passed to matrixReadyCallback as soon as there were enough samples averaged. If
     * support for the Eigen library is enabled the default matrix type will be an Eigen::Matrix, otherwise you need
     * to supply a MatrixType that is assignable via an overloaded operator() just like Eigen::Matrix.
     */
#if NTLAB_INCLUDE_EIGEN
    template <typename SampleType, typename MatrixType = Eigen::Matrix<std::complex<SampleType>, Eigen::Dynamic, Eigen::Dynamic>>
#else
    template <typename SampleType, typename MatrixType>
#endif
    class CLCovarianceMatrix
    {
    public:
        /**Constructs a covariance matrix instance. The number of samples to average can safely be changed at runtime. */
        CLCovarianceMatrix (int numSamplesToAverage, int numChannels)
                : numSampleDesired (numSamplesToAverage),
                  numChannelsExpected (numChannels),
                  covMatrix (numChannels, numChannels)
        {
            // some internal functionality depends on having at least one channel
            jassert (numChannels > 0);
            jassert (numSamplesToAverage >= 0);

            /* In theory, the matrix looks like that (4 x 4 mat for an example):
            X00  X01  X02  X03
            X10  X11  X12  X13
            X20  X21  X22  X23
            X30  X31  X32  X33

            But only these Values have to be computed:

            X00  X01  X02  X03
                 X11  X12  X13
                      X22  X23
                           X33

            So, in memory this will be arranged like that:
            X00  X11  X22  X33  X01  X02  X03  X12  X13  X23
            [tmpBuffer                                     ]
                                triangularRowStart[0]
                                               triangularRowStart[1]
                                                         triangularRowStart[2]

            The figure above tries to illustrate to what fields which member variables refer.

            The diagonal entries are completely real valued while the upper triangular entries are complex valued.
            Depending on the chosen kind of computation strategy (scalar or SIMD vectorized) each entry represents a
            scalar value or a complete SIMD vector.
            The tmpBuffer holds these values tightly packed, where complex values are represented by
            two scalars/vectors, one containing the real values followed by one containing the imaginary value(s).
            */

            numTriangularEntries = ((numChannels - 1) * numChannels) / 2;
            tmpBufferSize = (numChannels + 2 * numTriangularEntries)  * SIMDHelpers::VectorLength<SampleType>::numValues();
            tmpBuffer = SIMDHelpers::Allocation<SampleType>::allocateAlignedVector (tmpBufferSize);
            std::fill (tmpBuffer, tmpBuffer + tmpBufferSize, SampleType(0));

            triangularRowStart.resize (numTriangularEntries);
            triangularRowStart[0] = tmpBuffer + (numChannels * SIMDHelpers::VectorLength<SampleType>::numValues());
            int numEntriesCurrentLine = numChannels - 1;
            for (int i = 1; i < (numChannels - 1); ++i)
            {
                triangularRowStart[i] = triangularRowStart[i - 1] + numEntriesCurrentLine * 2 * SIMDHelpers::VectorLength<SampleType>::numValues();
                --numEntriesCurrentLine;
            }
        };

        ~CLCovarianceMatrix()
        {
            SIMDHelpers::Allocation<SampleType>::freeAlignedVector (tmpBuffer);
        }

        /**
         * Sets a new number of samples to average. Note that calling this function to decrease the number of samples
         * at a timepoint where alredy more samples were collected might lead to averaging over some number of samples
         * that lies between the old and the new number for one time.
         */
        void setNumSamplesToAverage (int newNumSamples) {numSampleDesired = newNumSamples; };

        /** Returns the number of samples that the algorithm currently uses for averaring */
        int getNumSamplesToAverage() {return numSampleDesired; };

        /**
         * This function will be invoked on the realtime thread during processNextSampleBlock as soon as the desired
         * number of samples to average over have been collected. For computational heavy task like DOA estimation you
         * should better just use the callback to copy the matrix and pass it to another thread.
         */
        std::function<void(MatrixType&)> matrixReadyCallback = [] (MatrixType&) {};

        /** Adds the most recent sample buffer to the covariance matrix */
        template <typename ComplexSampleBufferType>
        void processNextSampleBlock (const ComplexSampleBufferType& buffer)
        {
            static_assert (IsSampleBuffer<ComplexSampleBufferType, SampleType>::complex(), "Only SampleBufferComplex or CLSampleBufferComplex supported");

            jassert (buffer.getNumChannels() == numChannelsExpected);

            int numSamplesAvailable = buffer.getNumSamples();
            int startSampleIdx = 0;

            while (numSamplesAvailable)
            {
                const int numSamplesMissing   = numSampleDesired - numSamplesInCurrentMatrix;
                const int numSamplesToProcess = std::min (numSamplesMissing, numSamplesAvailable);

                processNextSampleBlockNonSIMD (buffer, startSampleIdx, numSamplesToProcessWithoutSIMD);

                numSamplesInCurrentMatrix += numSamplesToProcess;
                numSamplesAvailable       -= numSamplesToProcess;
                startSampleIdx            += numSamplesToProcessWithoutSIMD;

                if (numSamplesInCurrentMatrix >= numSampleDesired)
                {
                    int colStart = 1;
                    for (int row = 0; row < numChannelsExpected; ++row)
                    {
                        auto tmpBufferDiagPtr = getTmpBufferPtrForDiagonalEntry (row);
                        covMatrix(row, row) = std::accumulate (tmpBufferDiagPtr, tmpBufferDiagPtr + SIMDHelpers::VectorLength<SampleType>::numValues(), SampleType(0)) / numSamplesInCurrentMatrix;


                        for (int col = colStart; col < numChannelsExpected; ++col)
                        {
                            auto tmpBufferTriPtr = getTmpBufferPtrForTriangularEntry (row, col);

                            auto re = std::accumulate (tmpBufferTriPtr,                                                      tmpBufferTriPtr +      SIMDHelpers::VectorLength<SampleType>::numValues(),  SampleType(0));
                            auto im = std::accumulate (tmpBufferTriPtr + SIMDHelpers::VectorLength<SampleType>::numValues(), tmpBufferTriPtr + (2 * SIMDHelpers::VectorLength<SampleType>::numValues()), SampleType(0));

                            std::complex<SampleType> triEntry (re, im);
                            triEntry /= SampleType (numSamplesInCurrentMatrix);

                            covMatrix(row, col) =            triEntry;
                            covMatrix(col, row) = std::conj (triEntry);
                        }

                        ++colStart;
                    }

                    matrixReadyCallback (covMatrix);
                    numSamplesInCurrentMatrix = 0;
                    std::fill (tmpBuffer, tmpBuffer + tmpBufferSize, SampleType(0));
                }
            }
        }

    private:
        std::atomic<int> numSampleDesired;
        int numSamplesInCurrentMatrix = 0;
        const int numChannelsExpected;

        MatrixType covMatrix;

        SampleType* tmpBuffer = nullptr;
        int tmpBufferSize;
        int numTriangularEntries;
        std::vector<SampleType*> triangularRowStart;

        SampleType* getTmpBufferPtrForDiagonalEntry (int pos)
        {
            const int offset = SIMDHelpers::VectorLength<SampleType>::numValues() * pos;
            return tmpBuffer + offset;
        }

        SampleType* getTmpBufferPtrForTriangularEntry (int row, int col)
        {
            const int offsetFromTriangularRowStart = 2 * SIMDHelpers::VectorLength<SampleType>::numValues() * (col - row - 1);
            return triangularRowStart[row] + offsetFromTriangularRowStart;
        }


        template <typename ComplexSampleBufferType>
        void processNextSampleBlockNonSIMD (ComplexSampleBufferType& buffer, int startSampleIdx, int numSamplesToProcess)
        {
            const int initialStartSampleIdx = startSampleIdx;

            int chanBStart = 1;
            for (int chanA = 0; chanA < numChannelsExpected; ++chanA)
            {
                auto chanAPtr = const_cast<std::complex<SampleType>*> (buffer.getReadPointer (chanA) + startSampleIdx);
                auto tmpBufDiagPtr = getTmpBufferPtrForDiagonalEntry (chanA);

                for (int sampleIdx = 0; sampleIdx < numSamplesToProcess; ++sampleIdx)
                {
                    *tmpBufDiagPtr += (chanAPtr->real() * chanAPtr->real()) + (chanAPtr->imag() * chanAPtr->imag());

                    for (int chanB = chanBStart; chanB < numChannelsExpected; ++chanB)
                    {
                        auto chanBPtr = buffer.getReadPointer (chanB) + startSampleIdx;
                        auto chanAxChanB = *chanAPtr * std::conj(*chanBPtr);

                        float* previousResultRealPtr = getTmpBufferPtrForTriangularEntry (chanA, chanB);
                        float* previousResultImagPtr = previousResultRealPtr + SIMDHelpers::VectorLength<SampleType>::numValues();

                        *previousResultRealPtr += chanAxChanB.real();
                        *previousResultImagPtr += chanAxChanB.imag();
                    }

                    ++startSampleIdx;
                    ++chanAPtr;
                }

                ++chanBStart;
                startSampleIdx = initialStartSampleIdx;
            }
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CovarianceMatrix)
    };
}
