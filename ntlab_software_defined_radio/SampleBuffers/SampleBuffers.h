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

#include <complex>
#include <juce_core/juce_core.h>
#include "VectorOperations.h"

#include "../OpenCL2/cl2WithVersionChecks.h"

// macros to avoid a lot of boilerplate code
#define NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID jassert (numSamlesToCopy >= 0); \
                                                                jassert (numChannelsToCopy >= 0); \
                                                                jassert (sourceStartSample >= 0); \
                                                                jassert (destinationStartSample >= 0); \
                                                                jassert (sourceStartChannelNumber >= 0); \
                                                                jassert (destinationStartChannelNumber >= 0); \

#define NTLAB_OPERATION_ON_ALL_CHANNELS(functionToInvoke) for (int c = 0; c < numChannelsToCopy; ++c) \
                                                          { \
                                                              const auto readPtr = channelPtrs[sourceStartChannelNumber + c] + sourceStartSample; \
                                                              auto writePtr = otherBuffer.getWritePointer (destinationStartChannelNumber + c) + destinationStartSample; \
                                                              functionToInvoke; \
                                                          }

namespace ntlab
{
    // Forward declarations
    template <typename SampleType>
    class SampleBufferReal;

    template <typename SampleType>
    class SampleBufferComplex;

    template <typename SampleType>
    class CLSampleBufferReal;

    template <typename SampleType>
    class CLSampleBufferComplex;

    template <typename SampleType>
    class SampleBufferReal
    {
    public:

        /**
         * Constructs a SampleBufferReal and allocates heap memory for a buffer of the desired size. The memory is
         * managed by this instance, e.g. it gets deleted when the buffer goes out of scope.
         */
        SampleBufferReal (int numChannels, int numSamples, bool initWithZeros = false)
                : ownsBuffer (true),
                  numChannelsAllocated (numChannels),
                  numSamplesAllocated  (numSamples),
                  numSamplesUsed       (numSamples)
        {
            jassert (numChannels >= 0);
            jassert (numSamples >= 0);

            if (numChannels == 0)
            {
                channelPtrs = nullptr;
                return;
            }

            channelPtrs = new SampleType*[numChannels];

            for (int i = 0; i < numChannels; ++i)
                channelPtrs[i] = ntlab::SIMDHelpers::Allocation<SampleType>::allocateAlignedVector (numSamples);
        }

        /**
         * Constructs a SampleBufferReal referring to a raw buffer. Make sure that the lifetime of the buffer that it
         * refers to is greater than the lifetime of the instance created.
         * Note: Creating such a buffer is relatively lightweight task compared to creating a buffer that owns the
         * memory, so you might consider to create a temporary instance on the realtime processing thread.
         */
        SampleBufferReal (int numChannels, int numSamples, SampleType** bufferToReferTo)
                : ownsBuffer (false),
                  numChannelsAllocated (numChannels),
                  numSamplesAllocated  (numSamples),
                  numSamplesUsed       (numSamples),
                  channelPtrs             (bufferToReferTo)
        {
            jassert (numChannels >= 0);
            jassert (numSamples >= 0);
        }

        /** Move constructor */
        SampleBufferReal (SampleBufferReal&& other)
               : ownsBuffer (other.ownsBuffer),
                 numChannelsAllocated (other.numChannelsAllocated),
                 numSamplesAllocated (other.numSamplesAllocated),
                 numSamplesUsed (other.numSamplesUsed),
                 channelPtrs (other.channelPtrs)
        {
            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channelPtrs = nullptr;
        }

        /** Move assignment */
        SampleBufferReal& operator= (SampleBufferReal&& other)
        {
            ownsBuffer = other.ownsBuffer;
            numChannelsAllocated = other.numChannelsAllocated;
            numSamplesAllocated = other.numSamplesAllocated;
            numSamplesUsed = other.numSamplesUsed;
            channelPtrs = other.channelPtrs;

            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channelPtrs = nullptr;
        }

        ~SampleBufferReal()
        {
            if (ownsBuffer)
            {
                if (channelPtrs != nullptr)
                {
                    for (int c = 0; c < numChannelsAllocated; ++c)
                        ntlab::SIMDHelpers::Allocation<SampleType>::freeAlignedVector (channelPtrs[c]);

                    delete[] (channelPtrs);
                }
            }
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return false; }

        /**
         * Returns the number of valid samples per channel currently used. To get the maximum possible numer of samples
         * allocated by this buffer call getMaxNumSamples();
         *
         * @see getMaxNumSamples
         */
        const int getNumSamples() const {return numSamplesUsed; }

        /**
         * Returns the maximum number of samples per channel that can be held by this buffer. This equals the number of
         * samples passed to the buffers constructor.
         */
        const int getMaxNumSamples() const {return numSamplesAllocated; }

        /**
         * Sets the number of samples per channel current held by this buffer. This can be everything between 0 and
         * the maximum number of samples as returned by getMaxNumSamples.
         */
        void setNumSamples (int newNumSamples)
        {
            jassert (juce::isPositiveAndNotGreaterThan (newNumSamples, numSamplesAllocated));
            numSamplesUsed = newNumSamples;
        }

        /** Returns the number of channels held by this buffer */
        const int getNumChannels() const {return  numChannelsAllocated; }

        /**
         * Returns a read-only pointer to the host memory buffer for a dedicated channel. Always use this for read-only
         * operations. For speed reasons it does not check if the channelNumber is inside the valid range, so be careful
         * when using it.
         */
        const SampleType* getReadPointer (int channelNumber) const
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        SampleType* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const SampleType** getArrayOfReadPointers() const {return const_cast<const SampleType**> (channelPtrs); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        SampleType** getArrayOfWritePointers() {return channelPtrs; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channelPtrs[c] + startOfRegion, channelPtrs[c] + endOfRegion, SampleType (0));
        }

        /**
         * Copies the content of this buffer to another CLSampleBufferReal via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled for
         * the destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo(CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (SampleType)))
        }

        /**
         * Copies the content of this buffer to a SampleBufferReal. For speed reasons it does not check if the
         * parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (SampleType)))
        }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static SampleType castToSampleType (OriginalType sample) {return static_cast<SampleType> (sample); }


    private:
        bool ownsBuffer;

        int numChannelsAllocated;
        int numSamplesAllocated;
        int numSamplesUsed;

        SampleType** channelPtrs;

        JUCE_LEAK_DETECTOR (SampleBufferReal)
    };

    template <typename SampleType>
    class SampleBufferComplex
    {
    public:
        /**
         * Constructs a SampleBufferComplex and allocates heap memory for a buffer of the desired size. The memory is
         * managed by this instance, e.g. it gets deleted when the buffer goes out of scope.
         */
        SampleBufferComplex (int numChannels, int numSamples, bool initWithZeros = false)
          : ownsBuffer (true),
            numChannelsAllocated (numChannels),
            numSamplesAllocated  (numSamples),
            numSamplesUsed       (numSamples)
        {
            jassert (numChannels >= 0);
            jassert (numSamples >= 0);

            if (numChannels == 0)
            {
                channelPtrs = nullptr;
                return;
            }

            channelPtrs = new std::complex<SampleType>*[numChannels];

            for (int i = 0; i < numChannels; ++i)
                channelPtrs[i] = ntlab::SIMDHelpers::Allocation<std::complex<SampleType>>::allocateAlignedVector (numSamples);
        }

        /**
         * Constructs a SampleBufferComplex referring to a raw buffer. Make sure that the lifetime of the buffer that it
         * refers to is greater than the lifetime of the instance created.
         * Note: Creating such a buffer is relatively lightweight task compared to creating a buffer that owns the
         * memory, so you might consider to create a temporary instance on the realtime processing thread.
         */
        SampleBufferComplex (int numChannels, int numSamples, std::complex<SampleType>** bufferToReferTo)
          : ownsBuffer (false),
            numChannelsAllocated (numChannels),
            numSamplesAllocated  (numSamples),
            numSamplesUsed       (numSamples),
            channelPtrs             (bufferToReferTo)
        {
            jassert (numChannels >= 0);
            jassert (numSamples >= 0);
        }

        /** Move constructor */
        SampleBufferComplex (SampleBufferComplex&& other)
                : ownsBuffer (other.ownsBuffer),
                  numChannelsAllocated (other.numChannelsAllocated),
                  numSamplesAllocated (other.numSamplesAllocated),
                  numSamplesUsed (other.numSamplesUsed),
                  channelPtrs (other.channelPtrs)
        {
            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channelPtrs = nullptr;
        }

        /** Move assignment */
        SampleBufferComplex& operator= (SampleBufferComplex&& other)
        {
            ownsBuffer = other.ownsBuffer;
            numChannelsAllocated = other.numChannelsAllocated;
            numSamplesAllocated = other.numSamplesAllocated;
            numSamplesUsed = other.numSamplesUsed;
            channelPtrs = other.channelPtrs;

            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channelPtrs = nullptr;
        }

        ~SampleBufferComplex()
        {
            if (ownsBuffer)
            {
                if (channelPtrs != nullptr)
                {
                    for (int c = 0; c < numChannelsAllocated; ++c)
                        ntlab::SIMDHelpers::Allocation<std::complex<SampleType>>::freeAlignedVector (channelPtrs[c]);

                    delete[] (channelPtrs);
                }
            }
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return true; }

        /**
         * Returns the number of valid samples per channel currently used. To get the maximum possible numer of samples
         * allocated by this buffer call getMaxNumSamples();
         *
         * @see getMaxNumSamples
         */
        const int getNumSamples() const {return numSamplesUsed; }

        /**
         * Returns the maximum number of samples per channel that can be held by this buffer. This equals the number of
         * samples passed to the buffers constructor.
         */
        const int getMaxNumSamples() const {return numSamplesAllocated; }

        /**
         * Sets the number of samples per channel current held by this buffer. This can be everything between 0 and
         * the maximum number of samples as returned by getMaxNumSamples.
         */
        void setNumSamples (int newNumSamples)
        {
            jassert (juce::isPositiveAndNotGreaterThan (newNumSamples, numSamplesAllocated));
            numSamplesUsed = newNumSamples;
        }

        /** Returns the number of channels held by this buffer */
        const int getNumChannels() const {return  numChannelsAllocated; }

        /**
         * Returns a read-only pointer to the host memory buffer for a dedicated channel. Always use this for read-only
         * operations. For speed reasons it does not check if the channelNumber is inside the valid range, so be careful
         * when using it.
         */
        const std::complex<SampleType>* getReadPointer (int channelNumber) const
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        std::complex<SampleType>* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const std::complex<SampleType>** getArrayOfReadPointers() const {return const_cast<const std::complex<SampleType>**> (channelPtrs); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        std::complex<SampleType>** getArrayOfWritePointers() {return channelPtrs; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channelPtrs[c] + startOfRegion, channelPtrs[c] + endOfRegion, SampleType (0));
        }

        /**
         * Copies the content of this buffer to another CLSampleBufferComplex via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled for
         * the destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (std::complex<SampleType>)))
        }

        /**
         * Copies the content of this buffer to a SampleBufferComplex. For speed reasons it does not check if the
         * parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (std::complex<SampleType>)))
        }

        /**
         * Copies the real part of this buffer to another CLSampleBufferComplex via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled for
         * the destination buffer, so be careful when using it.
         *
         * @param otherBuffer                           Reference to the destination buffer
         * @param numSamlesToCopy                       Number of samples that should be copied
         * @param numChannelsToCopy                     Number of channels that should be copied
         * @param sourceStartSample                     The index of the first source sample for each channel copied
         * @param destinationStartSample                The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber              The index of the first source channel to copy
         * @param destinationStartChannelNumber         The index of the first destination channel to copy
         * @param clearImaginaryPartOfDestinationBuffer Set to true if you want the imaginary part of the destination
         *                                              buffer to be set to zero after copying
         */
        void copyRealPartTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearImaginaryPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            // todo
            jassertfalse;
        }

        /**
         * Copies the real part of this buffer to another SampleBufferComplex. For speed reasons it does not check
         * if the parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                           Reference to the destination buffer
         * @param numSamlesToCopy                       Number of samples that should be copied
         * @param numChannelsToCopy                     Number of channels that should be copied
         * @param sourceStartSample                     The index of the first source sample for each channel copied
         * @param destinationStartSample                The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber              The index of the first source channel to copy
         * @param destinationStartChannelNumber         The index of the first destination channel to copy
         * @param clearImaginaryPartOfDestinationBuffer Set to true if you want the imaginary part of the destination
         *                                              buffer to be set to zero after copying
         */
        void copyRealPartTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearImaginaryPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // todo
            jassertfalse;
        }

        /**
         * Copies the real part of this buffer to a CLSampleBufferReal via the host CPU. For speed reasons it does
         * neither check if the parameters passed are in the valid range, nor if host device access is enabled for the
         * destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyRealPartTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractRealPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Copies the real part of this buffer to a SampleBufferReal. For speed reasons it does not check if the
         * parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyRealPartTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractRealPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
          * Copies the imaginary part of this buffer to another CLSampleBufferComplex via the host CPU. For speed
          * reasons it does neither check if the parameters passed are in the valid range, nor if host device access is
          * enabled for the destination buffer, so be careful when using it.
          *
          * @param otherBuffer                      Reference to the destination buffer
          * @param numSamlesToCopy                  Number of samples that should be copied
          * @param numChannelsToCopy                Number of channels that should be copied
          * @param sourceStartSample                The index of the first source sample for each channel copied
          * @param destinationStartSample           The index of the first destination sample for each channel copied
          * @param sourceStartChannelNumber         The index of the first source channel to copy
          * @param destinationStartChannelNumber    The index of the first destination channel to copy
          * @param clearRealPartOfDestinationBuffer Set to true if you want the real part of the destination
          *                                         buffer to be set to zero after copying
          */
        void copyImaginaryPartTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearRealPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            //todo
            jassertfalse;
        }

        /**
         * Copies the imaginary part of this buffer to another SampleBufferComplex. For speed reasons it does not
         * check if the parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                      Reference to the destination buffer
         * @param numSamlesToCopy                  Number of samples that should be copied
         * @param numChannelsToCopy                Number of channels that should be copied
         * @param sourceStartSample                The index of the first source sample for each channel copied
         * @param destinationStartSample           The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber         The index of the first source channel to copy
         * @param destinationStartChannelNumber    The index of the first destination channel to copy
         * @param clearRealPartOfDestinationBuffer Set to true if you want the real part of the destination
         *                                         buffer to be set to zero after copying
         */
        void copyImaginaryPartTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearRealPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            //todo
            jassertfalse;
        }

        /**
         * Copies the imaginary part of this buffer to a CLSampleBufferReal via the host CPU. For speed reasons it does
         * neither check if the parameters passed are in the valid range, nor if host device access is enabled for the
         * destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyImaginaryPartTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractImagPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Copies the imaginary part of this buffer to a SampleBufferReal. For speed reasons it does not check if
         * the parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyImaginaryPartTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractImagPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Computes the absolute values of this buffer and copies them to a CLSampleBufferReal via the host CPU. For
         * speed reasons it does neither check if the parameters passed are in the valid range, nor if host device
         * access is enabled for the destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyAbsoluteValuesTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::abs (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Computes the absolute values of this buffer and copies them to a SampleBufferReal. For speed reasons it does
         * not check if the parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyAbsoluteValuesTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::abs (readPtr, writePtr, numSamlesToCopy))
        }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static std::complex<SampleType> castToSampleType (std::complex<OriginalType> sample) {return {static_cast<SampleType> (sample.real()), static_cast<SampleType> (sample.imag())}; }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static std::complex<SampleType> castToSampleType (OriginalType re, OriginalType im) {return {static_cast<SampleType> (re), static_cast<SampleType> (im)}; }

    private:
        bool ownsBuffer;

        int numChannelsAllocated;
        int numSamplesAllocated;
        int numSamplesUsed;

        std::complex<SampleType>** channelPtrs;

        JUCE_LEAK_DETECTOR (SampleBufferComplex)
    };


    template <typename SampleType>
    class CLSampleBufferReal
    {
    public:

        CLSampleBufferReal (const int numChannels,
                            const int numSamples,
                            cl::CommandQueue& queueToUse,
                            cl::Context& contextToUse,
                            bool initWithZeros = false,
                            cl_mem_flags clMemAccessFlags = CL_MEM_READ_WRITE,
                            cl_map_flags clMapFlags = CL_MAP_READ | CL_MAP_WRITE) 
          : queue       (queueToUse),
            context     (contextToUse), 
            mapFlags    (clMapFlags),
            numChannelsAllocated (numChannels),
            numSamplesAllocated  (numSamples),
            numSamplesUsed       (numSamples),
            channelPtrs (static_cast<size_t> (numChannels))
        {
            jassert (numChannels >= 0);
            jassert (numSamples  >= 0);

            cl_int err;
            numBytesInBuffer = numSamples * numChannels * sizeof (SampleType);

            clBuffer = cl::Buffer (context, CL_MEM_ALLOC_HOST_PTR | clMemAccessFlags,  numBytesInBuffer);
            SampleType* mappedBufferStart = queue.enqueueMapBuffer (clBuffer, CL_TRUE, mapFlags, 0, numBytesInBuffer, nullptr, nullptr, &err);

            // Something went wrong when trying to map the CL memory
            jassert (err == CL_SUCCESS);

            std::vector<cl_int> channelListIdx (numChannels);
            for (int i = 0; i < numChannels; ++i)
            {
                channelPtrs[i]    = mappedBufferStart + (i * numSamples);
                channelListIdx[i] = (i * numSamples);

                // Please report an issue on GitHub if this assert fires
                jassert (SIMDHelpers::isPointerAligned (channelPtrs[i]));
            }

            if (initWithZeros)
                clearBufferRegion();

            channelList = cl::Buffer (context, CL_MEM_READ_ONLY, numChannels * sizeof (cl_int));
            queue.enqueueWriteBuffer (channelList, CL_TRUE,   0, numChannels * sizeof (cl_int), channelListIdx.data());
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return false; }

        /** For FPGA targets with shared memory architecture the memory is always mapped */
        static constexpr bool isAlwaysMapped()
        {
#ifdef OPEN_CL_INTEL_FPGA
            return true;
#else
            return false;
#endif
        }

        /**
         * Enables access of the buffer for the host. You can chose between a blocking call that waits until the
         * operation has been finished (which might involve a memory copy between device and host memory on
         * systems that don't use a shared memory architecture) or a non-blocking call. You can furthermore specify
         * a vector of events that must have been finished before the mapping takes place and an event that is created
         * to monitor the execution status of the mapping. Note that on most platforms events and event callbacks lead
         * to memory allocation so you are better of not using these from any real-time thread.
         * Return CL_SUCCESS if mapping was successful or an error code in case of no success.
         */
        cl_int mapHostMemory (cl_bool blocking = CL_TRUE, cl::vector<cl::Event>* eventsToWaitFor = nullptr, cl::Event* eventToCreate = nullptr)
        {
            if constexpr (isAlwaysMapped())
                return CL_SUCCESS;

            if (isMapped)
                return CL_SUCCESS;

            cl_int err;
            SampleType* mappedBufferStart = queue.enqueueMapBuffer (clBuffer, blocking, mapFlags, 0, numBytesInBuffer, eventsToWaitFor, eventToCreate, &err);
            for (int i = 0; i < numChannelsAllocated; ++i)
            {
                channelPtrs[i]    = mappedBufferStart + (i * numSamplesAllocated);

                // Please report an issue on GitHub if this assert fires
                jassert (SIMDHelpers::isPointerAligned (channelPtrs[i]));
            }

            return err;
        }

        /**
         * Disables access of the buffer for the host to pass it to a CL kernel. You can specify a vector of events
         * that must have been finished before the mapping takes place and an event that is created to monitor the
         * execution status of the mapping. Note that on most platforms events and event callbacks lead to memory
         * allocation so you are better of not using these from any real-time thread.
         * Return CL_SUCCESS if unmapping was successful or an error code in case of no success.
         */
        cl_int unmapHostMemory (cl::vector<cl::Event>* eventsToWaitFor = nullptr, cl::Event* eventToCreate = nullptr)
        {
            if constexpr (isAlwaysMapped())
                return CL_SUCCESS;

            if (!isMapped)
                return CL_SUCCESS;

            return queue.enqueueUnmapMemObject (clBuffer, channelPtrs[0], eventsToWaitFor, eventToCreate);
        }

        /** Returns true if host access is enabled, false if CL device access is enabled. */
        constexpr bool isCurrentlyMapped() const {return isAlwaysMapped() || isMapped; }

        /**
         * Returns the number of valid samples per channel currently used. To get the maximum possible numer of samples
         * allocated by this buffer call getMaxNumSamples();
         *
         * @see getMaxNumSamples
         */
        const int getNumSamples() const {return numSamplesUsed; }

        /**
         * Returns the maximum number of samples per channel that can be held by this buffer. This equals the number of
         * samples passed to the buffers constructor.
         */
        const int getMaxNumSamples() const {return numSamplesAllocated; }

        /**
         * Sets the number of samples per channel current held by this buffer. This can be everything between 0 and
         * the maximum number of samples as returned by getMaxNumSamples.
         */
        void setNumSamples (int newNumSamples)
        {
            jassert (juce::isPositiveAndNotGreaterThan (newNumSamples, numSamplesAllocated));
            numSamplesUsed = newNumSamples;
        }

        /** Returns the number of channels held by this buffer */
        const int getNumChannels() const {return  numChannelsAllocated; }

        /**
         * Returns a read-only pointer to the host memory buffer for a dedicated channel. Always use this for read-only
         * operations. For speed reasons it does not check if the channelNumber is inside the valid range, so be careful
         * when using it.
         */
        const SampleType* getReadPointer (int channelNumber) const
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        SampleType* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const SampleType** getArrayOfReadPointers() const {return const_cast<const SampleType**> (channelPtrs.data()); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        SampleType** getArrayOfWritePointers() {return channelPtrs.data(); }

        /**
         * Handy cast operator to pass the buffer to the kernel, for mapping/unmapping you should use mapHostMemory
         * and unmapHostMemory
         */
        operator cl::Buffer&() {return clBuffer; }

        /**
         * Returns a cl:Buffer object that holds the indexes to the beginning of the single channel regions in the
         * underlying cl:Buffer containing the samples. This won't change during the whole object lifetime. You might
         * want to pass this and the number of samples used along with the actual sample buffer to your kernel
         * @return
         */
        const cl::Buffer& getCLChannelList() const {return channelList; }

        /** Get the queue associated with this buffer. You can use this to enqueue your kernel. */
        cl::CommandQueue& getQueueAssociatedWithThisBuffer() const {return queue; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            jassert (isMapped);
            
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channelPtrs[c] + startOfRegion, channelPtrs[c] + endOfRegion, SampleType (0));
        }

        /**
         * Copies the content of this buffer to another CLSampleBufferReal via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled for
         * the destination buffer, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (SampleType)))
        }

        /**
         * Copies the content of this buffer to a SampleBufferReal. For speed reasons it does not check if the
         * parameters passed are in the valid range, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (SampleType)))
        }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static SampleType castToSampleType (OriginalType sample) {return static_cast<SampleType> (sample); }

    private:
        int numChannelsAllocated;
        int numSamplesAllocated;
        int numSamplesUsed;

        cl::CommandQueue& queue;
        cl::Context&      context;
        cl::Buffer        clBuffer;
        cl::Buffer        channelList;
        cl_map_flags      mapFlags;
        size_t            numBytesInBuffer;

        std::vector<SampleType*> channelPtrs;
        bool isMapped = true;

        JUCE_LEAK_DETECTOR (CLSampleBufferReal)
    };

    template <typename SampleType>
    class CLSampleBufferComplex
    {
    public:

        CLSampleBufferComplex (const int numChannels,
                               const int numSamples,
                               cl::CommandQueue& queueToUse,
                               cl::Context& contextToUse,
                               bool initWithZeros = false,
                               cl_mem_flags clMemAccessFlags = CL_MEM_READ_WRITE,
                               cl_map_flags clMapFlags = CL_MAP_READ | CL_MAP_WRITE)
          : queue       (queueToUse),
            context     (contextToUse),
            mapFlags    (clMapFlags),
            numChannelsAllocated (numChannels),
            numSamplesAllocated  (numSamples),
            numSamplesUsed       (numSamples),
            channelPtrs (static_cast<size_t> (numChannels))
        {
            jassert (numChannels >= 0);
            jassert (numSamples  >= 0);

            cl_int err;
            numBytesInBuffer = numSamples * numChannels * sizeof (std::complex<SampleType>);

            clBuffer = cl::Buffer (context, CL_MEM_ALLOC_HOST_PTR | clMemAccessFlags, numBytesInBuffer);
            std::complex<SampleType>* mappedBufferStart = reinterpret_cast<std::complex<SampleType>*> (queue.enqueueMapBuffer (clBuffer, CL_TRUE, mapFlags, 0, numBytesInBuffer, nullptr, nullptr, &err));

            // Something went wrong when trying to map the CL memory
            jassert (err == CL_SUCCESS);

            std::vector<cl_int> channelListIdx (numChannels);
            for (int i = 0; i < numChannels; ++i)
            {
                channelPtrs[i]    = mappedBufferStart + (i * numSamples);
                channelListIdx[i] = (i * numSamples);

                // Please report an issue on GitHub if this assert fires
                jassert (SIMDHelpers::isPointerAligned (channelPtrs[i]));
            }

            if (initWithZeros)
                clearBufferRegion();

            channelList = cl::Buffer (context, CL_MEM_READ_ONLY, numChannels * sizeof (cl_int));
            queue.enqueueWriteBuffer (channelList, CL_TRUE,   0, numChannels * sizeof (cl_int), channelListIdx.data());
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return true; }

        /** For FPGA targets with shared memory architecture the memory is always mapped */
        static constexpr bool isAlwaysMapped()
        {
#ifdef OPEN_CL_INTEL_FPGA
            return true;
#else
            return false;
#endif
        }

        /**
         * Enables access of the buffer for the host. You can chose between a blocking call that waits until the
         * operation has been finished (which might involve a memory copy between device and host memory on
         * systems that don't use a shared memory architecture) or a non-blocking call. You can furthermore specify
         * a vector of events that must have been finished before the mapping takes place and an event that is created
         * to monitor the execution status of the mapping. Note that on most platforms events and event callbacks lead
         * to memory allocation so you are better of not using these from any real-time thread.
         * Return CL_SUCCESS if mapping was successful or an error code in case of no success.
         */
        cl_int mapHostMemory (cl_bool blocking = CL_TRUE, cl::vector<cl::Event>* eventsToWaitFor = nullptr, cl::Event* eventToCreate = nullptr)
        {
            if constexpr (isAlwaysMapped())
                return CL_SUCCESS;

            if (isMapped)
                return CL_SUCCESS;

            cl_int err;
            SampleType* mappedBufferStart = queue.enqueueMapBuffer (clBuffer, blocking, mapFlags, 0, numBytesInBuffer, eventsToWaitFor, eventToCreate, &err);
            for (int i = 0; i < numChannelsAllocated; ++i)
            {
                channelPtrs[i]    = mappedBufferStart + (i * numSamplesAllocated);

                // Please report an issue on GitHub if this assert fires
                jassert (SIMDHelpers::isPointerAligned (channelPtrs[i]));
            }

            return err;
        }

        /**
         * Disables access of the buffer for the host to pass it to a CL kernel. You can specify a vector of events
         * that must have been finished before the mapping takes place and an event that is created to monitor the
         * execution status of the mapping. Note that on most platforms events and event callbacks lead to memory
         * allocation so you are better of not using these from any real-time thread.
         * Return CL_SUCCESS if unmapping was successful or an error code in case of no success.
         */
        cl_int unmapHostMemory (cl::vector<cl::Event>* eventsToWaitFor = nullptr, cl::Event* eventToCreate = nullptr)
        {
            if constexpr (isAlwaysMapped())
                return CL_SUCCESS;

            if (!isMapped)
                return CL_SUCCESS;

            return queue.enqueueUnmapMemObject (clBuffer, channelPtrs[0], eventsToWaitFor, eventToCreate);
        }

        /**
         * Returns true if host device access is enabled, false if CL device access is enabled.
         */
        constexpr bool isCurrentlyMapped() const {return isMapped; }

        /**
         * Returns the number of valid samples per channel currently used. To get the maximum possible numer of samples
         * allocated by this buffer call getMaxNumSamples();
         *
         * @see getMaxNumSamples
         */
        const int getNumSamples() const {return numSamplesUsed; }

        /**
         * Returns the maximum number of samples per channel that can be held by this buffer. This equals the number of
         * samples passed to the buffers constructor.
         */
        const int getMaxNumSamples() const {return numSamplesAllocated; }

        /**
         * Sets the number of samples per channel current held by this buffer. This can be everything between 0 and
         * the maximum number of samples as returned by getMaxNumSamples.
         */
        void setNumSamples (int newNumSamples)
        {
            jassert (juce::isPositiveAndNotGreaterThan (newNumSamples, numSamplesAllocated));
            numSamplesUsed = newNumSamples;
        }

        /** Returns the number of channels held by this buffer */
        const int getNumChannels() const {return  numChannelsAllocated; }

        /**
         * Returns a read-only pointer to the host memory buffer for a dedicated channel. Always use this for read-only
         * operations. For speed reasons it does neither check if the channelNumber is inside the valid range nor if
         * host device access is enabled, so be careful when using it.
         */
        const std::complex<SampleType>* getReadPointer (int channelNumber) const
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does neither check if the channelNumber is inside
         * the valid range nor if host device access is enabled, so be careful when using it.
         */
        std::complex<SampleType>* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channelPtrs[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations. For speed reasons it does not check if host device access is enabled, so be careful
         * when using it.
         */
        const std::complex<SampleType>** getArrayOfReadPointers() const { return const_cast<const std::complex<SampleType>**> (channelPtrs.data()); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer. For speed reasons it does not check if host device access is
         * enabled, so be careful when using it.
         */
        std::complex<SampleType>** getArrayOfWritePointers() { return channelPtrs.data(); }

        /**
         * Handy cast operator to pass the buffer to the kernel, for mapping/unmapping you should use mapHostMemory
         * and unmapHostMemory
         */
        operator cl::Buffer&() {return clBuffer; }

        /**
         * Returns a cl:Buffer object that holds the indexes to the beginning of the single channel regions in the
         * underlying cl:Buffer containing the samples. This won't change during the whole object lifetime. You might
         * want to pass this and the number of samples used along with the actual sample buffer to your kernel
         * @return
         */
        const cl::Buffer& getCLChannelList() const {return channelList; }

        /** Get the queue associated with this buffer. You can use this to enqueue your kernel. */
        cl::CommandQueue& getQueueAssociatedWithThisBuffer() const {return queue; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            jassert (isMapped);

            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channelPtrs[c] + startOfRegion, channelPtrs[c] + endOfRegion, SampleType (0));
        }

        /**
         * Copies the content of this buffer to another CLSampleBufferComplex via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled, so
         * be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (std::complex<SampleType>)))
        }

        /**
         * Copies the content of this buffer to a SampleBufferComplex. For speed reasons it does neither check if the
         * parameters passed are in the valid range, nor if host device access is enabled, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (std::memcpy (writePtr, readPtr, numSamlesToCopy * sizeof (std::complex<SampleType>)))
        }

        /**
         * Copies the real part of this buffer to another CLSampleBufferComplex via the host CPU. For speed reasons it
         * does neither check if the parameters passed are in the valid range, nor if host device access is enabled, so
         * be careful when using it.
         *
         * @param otherBuffer                           Reference to the destination buffer
         * @param numSamlesToCopy                       Number of samples that should be copied
         * @param numChannelsToCopy                     Number of channels that should be copied
         * @param sourceStartSample                     The index of the first source sample for each channel copied
         * @param destinationStartSample                The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber              The index of the first source channel to copy
         * @param destinationStartChannelNumber         The index of the first destination channel to copy
         * @param clearImaginaryPartOfDestinationBuffer Set to true if you want the imaginary part of the destination
         *                                              buffer to be set to zero after copying
         */
        void copyRealPartTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearImaginaryPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            // todo
            jassertfalse;
        }

        /**
         * Copies the real part of this buffer to another SampleBufferComplex. For speed reasons it does neither check
         * if the parameters passed are in the valid range, nor if host device access is enabled, so be careful when
         * using it.
         *
         * @param otherBuffer                           Reference to the destination buffer
         * @param numSamlesToCopy                       Number of samples that should be copied
         * @param numChannelsToCopy                     Number of channels that should be copied
         * @param sourceStartSample                     The index of the first source sample for each channel copied
         * @param destinationStartSample                The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber              The index of the first source channel to copy
         * @param destinationStartChannelNumber         The index of the first destination channel to copy
         * @param clearImaginaryPartOfDestinationBuffer Set to true if you want the imaginary part of the destination
         *                                              buffer to be set to zero after copying
         */
        void copyRealPartTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearImaginaryPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            // todo
            jassertfalse;
        }

        /**
         * Copies the real part of this buffer to a CLSampleBufferReal via the host CPU. For speed reasons it does
         * neither check if the parameters passed are in the valid range, nor if host device access is enabled, so be
         * careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyRealPartTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractRealPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Copies the real part of this buffer to a SampleBufferReal. For speed reasons it does neither check if the
         * parameters passed are in the valid range, nor if host device access is enabled, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyRealPartTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractRealPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
          * Copies the imaginary part of this buffer to another CLSampleBufferComplex via the host CPU. For speed
          * reasons it does neither check if the parameters passed are in the valid range, nor if host device access is
          * enabled, so be careful when using it.
          *
          * @param otherBuffer                      Reference to the destination buffer
          * @param numSamlesToCopy                  Number of samples that should be copied
          * @param numChannelsToCopy                Number of channels that should be copied
          * @param sourceStartSample                The index of the first source sample for each channel copied
          * @param destinationStartSample           The index of the first destination sample for each channel copied
          * @param sourceStartChannelNumber         The index of the first source channel to copy
          * @param destinationStartChannelNumber    The index of the first destination channel to copy
          * @param clearRealPartOfDestinationBuffer Set to true if you want the real part of the destination
          *                                         buffer to be set to zero after copying
          */
        void copyImaginaryPartTo (CLSampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearRealPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            //todo
            jassertfalse;
        }

        /**
         * Copies the imaginary part of this buffer to another SampleBufferComplex. For speed reasons it does neither
         * check if the parameters passed are in the valid range, nor if host device access is enabled, so be careful
         * when using it.
         *
         * @param otherBuffer                      Reference to the destination buffer
         * @param numSamlesToCopy                  Number of samples that should be copied
         * @param numChannelsToCopy                Number of channels that should be copied
         * @param sourceStartSample                The index of the first source sample for each channel copied
         * @param destinationStartSample           The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber         The index of the first source channel to copy
         * @param destinationStartChannelNumber    The index of the first destination channel to copy
         * @param clearRealPartOfDestinationBuffer Set to true if you want the real part of the destination
         *                                         buffer to be set to zero after copying
         */
        void copyImaginaryPartTo (SampleBufferComplex<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0,
                bool clearRealPartOfDestinationBuffer = true) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            //todo
            jassertfalse;
        }

        /**
         * Copies the imaginary part of this buffer to a CLSampleBufferReal via the host CPU. For speed reasons it does
         * neither check if the parameters passed are in the valid range, nor if host device access is enabled, so be
         * careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyImaginaryPartTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractImagPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Copies the imaginary part of this buffer to a SampleBufferReal. For speed reasons it does neither check if
         * the parameters passed are in the valid range, nor if host device access is enabled, so be careful when using
         * it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyImaginaryPartTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::extractImagPart (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Computes the absolute values of this buffer and copies them to a CLSampleBufferReal via the host CPU. For
         * speed reasons it does neither check if the parameters passed are in the valid range, nor if host device
         * access is enabled, so be careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyAbsoluteValuesTo (CLSampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // both buffers need to be mapped to be copied in host memory space
            jassert (isMapped);
            jassert (otherBuffer.isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::abs (readPtr, writePtr, numSamlesToCopy))
        }

        /**
         * Computes the absolute values of this buffer and copies them to a SampleBufferReal. For speed reasons it does
         * neither check if the parameters passed are in the valid range, nor if host device access is enabled, so be
         * careful when using it.
         *
         * @param otherBuffer                   Reference to the destination buffer
         * @param numSamlesToCopy               Number of samples that should be copied
         * @param numChannelsToCopy             Number of channels that should be copied
         * @param sourceStartSample             The index of the first source sample for each channel copied
         * @param destinationStartSample        The index of the first destination sample for each channel copied
         * @param sourceStartChannelNumber      The index of the first source channel to copy
         * @param destinationStartChannelNumber The index of the first destination channel to copy
         */
        void copyAbsoluteValuesTo (SampleBufferReal<SampleType>& otherBuffer,
                int numSamlesToCopy,
                int numChannelsToCopy,
                int sourceStartSample = 0,
                int destinationStartSample = 0,
                int sourceStartChannelNumber = 0,
                int destinationStartChannelNumber = 0) const
        {
            NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
            // buffer needs to be mapped to be copied in host memory space
            jassert (isMapped);
            NTLAB_OPERATION_ON_ALL_CHANNELS (ComplexVectorOperations::abs (readPtr, writePtr, numSamlesToCopy))
        }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static std::complex<SampleType> castToSampleType (std::complex<OriginalType> sample) {return {static_cast<SampleType> (sample.real()), static_cast<SampleType> (sample.imag())}; }

        /** Helper to create one Sample of the buffers SampleType in templated code */
        template <typename OriginalType>
        static std::complex<SampleType> castToSampleType (OriginalType re, OriginalType im) {return {static_cast<SampleType> (re), static_cast<SampleType> (im)}; }

    private:
        int numChannelsAllocated;
        int numSamplesAllocated;
        int numSamplesUsed;

        cl::CommandQueue& queue;
        cl::Context&      context;
        cl::Buffer        clBuffer;
        cl::Buffer        channelList;
        cl_map_flags      mapFlags;
        size_t            numBytesInBuffer;

        std::vector<std::complex<SampleType>*> channelPtrs;
        bool isMapped = true;

        JUCE_LEAK_DETECTOR (CLSampleBufferComplex)
    };

    /**
     * A placeholder to be used as template parameter for IsSampleBuffer if you want to check if a buffer contains
     * either float, double, int16 or int32 samples.
     */
    struct AllValidSampleTypes;

    /**
     * A handy helper class if you write templated DSP functions that accept some kind of SampleBuffer. You can use it
     * this way
     *
     * Example 1: Your DSP class only accepts buffer with a certain SampleType same to the DSP class template type
     * @code
     * template <typename SampleType>
     * class MyDSPClass
     * {
     * public:
     *     template <typename BufferType>
     *     void processRealBuffer (BufferType& buffer)
     *     {
     *         static_assert (IsSampleBuffer<BufferType, SampleType>::real(), "Only real valued buffers supported");
     *     }
     *
     *     template <typename BufferType>
     *     void processCplxBuffer (BufferType& buffer)
     *     {
     *         static_assert (IsSampleBuffer<BufferType, SampleType>::complex(), "Only complex valued buffers supported");
     *     }
     * }
     * @endcode
     *
     * Example 2: Your DSP class will accept any buffer type. This works as the default parameter for ExpectedSampleType
     * is AllValidSampleTypes.
     * @code
     * class MyDSPClass
     * {
     * public:
     *     template <typename BufferType>
     *     void processRealBuffer (BufferType& buffer)
     *     {
     *         static_assert (IsSampleBuffer<BufferType>::complexOrReal(), "Only SampleBuffers supported");
     *     }
     * @endcode
     */
    template <typename BufferTypeToCheck, typename ExpectedSampleType = AllValidSampleTypes>
    struct IsSampleBuffer
    {
        /** Returns true if it is a SampleBufferReal<ExpectedSampleType> or CLSampleBufferReal<ExpectedSampleType> */
        static constexpr bool real()
        {
            if constexpr (std::is_same<ExpectedSampleType, AllValidSampleTypes>::value)
            {
                return std::is_same<BufferTypeToCheck, SampleBufferReal<float>>::value     ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferReal<float>>::value   ||
                       std::is_same<BufferTypeToCheck, SampleBufferReal<double>>::value    ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferReal<double>>::value  ||
                       std::is_same<BufferTypeToCheck, SampleBufferReal<int16_t>>::value   ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferReal<int16_t>>::value ||
                       std::is_same<BufferTypeToCheck, SampleBufferReal<int32_t>>::value   ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferReal<int32_t>>::value;
            }
            return std::is_same<BufferTypeToCheck, SampleBufferReal<ExpectedSampleType>>::value ||
                   std::is_same<BufferTypeToCheck, CLSampleBufferReal<ExpectedSampleType>>::value;
        }

        /** Returns true if it is a SampleBufferComplex<ExpectedSampleType> or CLSampleBufferComplex<ExpectedSampleType> */
        static constexpr bool complex()
        {
            if constexpr (std::is_same<ExpectedSampleType, AllValidSampleTypes>::value)
            {
                return std::is_same<BufferTypeToCheck, SampleBufferComplex<float>>::value     ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferComplex<float>>::value   ||
                       std::is_same<BufferTypeToCheck, SampleBufferComplex<double>>::value    ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferComplex<double>>::value  ||
                       std::is_same<BufferTypeToCheck, SampleBufferComplex<int16_t>>::value   ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferComplex<int16_t>>::value ||
                       std::is_same<BufferTypeToCheck, SampleBufferComplex<int32_t>>::value   ||
                       std::is_same<BufferTypeToCheck, CLSampleBufferComplex<int32_t>>::value;
            }
            return std::is_same<BufferTypeToCheck, SampleBufferComplex<ExpectedSampleType>>::value ||
                   std::is_same<BufferTypeToCheck, CLSampleBufferComplex<ExpectedSampleType>>::value;
        }

        /** Returns true if it is one of the four SampleBuffer classes with the expected sample type */
        static constexpr bool complexOrReal()
        {
            return real() || complex();
        }

        static constexpr bool cl()
        {
            if constexpr (std::is_same<ExpectedSampleType, AllValidSampleTypes>::value)
            {
                return  std::is_same<BufferTypeToCheck, CLSampleBufferComplex<float>>::value   ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferReal<float>>::value      ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferComplex<double>>::value  ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferReal<double>>::value     ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferComplex<int16_t>>::value ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferReal<int16_t>>::value    ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferComplex<int32_t>>::value ||
                        std::is_same<BufferTypeToCheck, CLSampleBufferReal<int32_t>>::value;
            }
            return std::is_same<BufferTypeToCheck, CLSampleBufferComplex<ExpectedSampleType>>::value ||
                   std::is_same<BufferTypeToCheck, CLSampleBufferReal<ExpectedSampleType>>::value;
        }

    };
}

#undef NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
#undef NTLAB_OPERATION_ON_ALL_CHANNELS