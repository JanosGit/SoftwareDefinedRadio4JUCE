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

#include <OpenCL/opencl.h>
#include <complex>
#include <juce_core/juce_core.h>
#include "VectorOperations.h"

// macros to avoid a lot of boilerplate code
#define NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID jassert (numSamlesToCopy >= 0); \
                                                                jassert (numChannelsToCopy >= 0); \
                                                                jassert (sourceStartSample >= 0); \
                                                                jassert (destinationStartSample >= 0); \
                                                                jassert (sourceStartChannelNumber >= 0); \
                                                                jassert (destinationStartChannelNumber >= 0); \

#define NTLAB_OPERATION_ON_ALL_CHANNELS(functionToInvoke) for (int c = 0; c < numChannelsToCopy; ++c) \
                                                          { \
                                                              const auto readPtr = channels[sourceStartChannelNumber + c] + sourceStartSample; \
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
                channels = nullptr;
                return;
            }

            channels = new SampleType*[numChannels];

            for (int i = 0; i < numChannels; ++i)
                channels[i] = ntlab::SIMDHelpers::Allocation<SampleType>::allocateAlignedVector (numSamples);
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
                  channels             (bufferToReferTo)
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
                 channels (other.channels)
        {
            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channels = nullptr;
        }

        /** Move assignment */
        SampleBufferReal& operator= (SampleBufferReal&& other)
        {
            ownsBuffer = other.ownsBuffer;
            numChannelsAllocated = other.numChannelsAllocated;
            numSamplesAllocated = other.numSamplesAllocated;
            numSamplesUsed = other.numSamplesUsed;
            channels = other.channels;

            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channels = nullptr;
        }

        ~SampleBufferReal()
        {
            if (ownsBuffer)
            {
                if (channels != nullptr)
                {
                    for (int c = 0; c < numChannelsAllocated; ++c)
                        ntlab::SIMDHelpers::Allocation<SampleType>::freeAlignedVector (channels[c]);

                    delete[] (channels);
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
            return channels[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        SampleType* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channels[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const SampleType** getArrayOfReadPointers() const {return const_cast<const SampleType**> (channels); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        SampleType** getArrayOfWritePointers() {return channels; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channels[c] + startOfRegion, channels[c] + endOfRegion, SampleType (0));
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

        SampleType** channels;

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
                channels = nullptr;
                return;
            }

            channels = new std::complex<SampleType>*[numChannels];

            for (int i = 0; i < numChannels; ++i)
                channels[i] = ntlab::SIMDHelpers::Allocation<std::complex<SampleType>>::allocateAlignedVector (numSamples);
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
            channels             (bufferToReferTo)
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
                  channels (other.channels)
        {
            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channels = nullptr;
        }

        /** Move assignment */
        SampleBufferComplex& operator= (SampleBufferComplex&& other)
        {
            ownsBuffer = other.ownsBuffer;
            numChannelsAllocated = other.numChannelsAllocated;
            numSamplesAllocated = other.numSamplesAllocated;
            numSamplesUsed = other.numSamplesUsed;
            channels = other.channels;

            other.ownsBuffer = false;
            other.numChannelsAllocated = 0;
            other.numSamplesAllocated = 0;
            other.numSamplesUsed = 0;
            other.channels = nullptr;
        }

        ~SampleBufferComplex()
        {
            if (ownsBuffer)
            {
                if (channels != nullptr)
                {
                    for (int c = 0; c < numChannelsAllocated; ++c)
                        ntlab::SIMDHelpers::Allocation<std::complex<SampleType>>::freeAlignedVector (channels[c]);

                    delete[] (channels);
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
            return channels[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        std::complex<SampleType>* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channels[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const std::complex<SampleType>** getArrayOfReadPointers() const {return const_cast<const std::complex<SampleType>**> (channels); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        std::complex<SampleType>** getArrayOfWritePointers() {return channels; }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channels[c] + startOfRegion, channels[c] + endOfRegion, SampleType (0));
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

        std::complex<SampleType>** channels;

        JUCE_LEAK_DETECTOR (SampleBufferComplex)
    };


    template <typename SampleType>
    class CLSampleBufferReal
    {
    public:

        CLSampleBufferReal()
        {
            // not implemented yet
            jassertfalse;
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return false; }

        /**
         * Enables access of the buffer for the host processor. This call might block until kernel executions in
         * progress on the CL device have been finished and might involve a memory copy from the CL device memory space
         * back to the host processors memory space in case of a non-shared memory architecture.
         */
        void enableHostDeviceAccess()
        {

        }

        /**
         * Enables access of the buffer for the host processor only if it is currently not used by any CL kernel.
         * Returns true if access could be enabled, false otherwise. This call might involve a memory copy from the CL
         * device memory space back to the host processors memory space in case of a non-shared memory architecture.
         */
        bool tryEnableHostDeviceAccess()
        {

        }

        /**
         * Disables access of the buffer for the host processor to pass it to a CL kernel.
         */
        void enableCLDeviceAccess()
        {

        }

        /**
         * Returns true if host device access is enabled, false if CL device access is enabled.
         */
        bool getCurrentAccessMode() const {return isInHostAccessMode; }

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
            return channels[channelNumber];
        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does not check if the channelNumber is inside
         * the valid range, so be careful when using it.
         */
        SampleType* getWritePointer (int channelNumber)
        {
            jassert (juce::isPositiveAndBelow (channelNumber, numChannelsAllocated));
            return channels[channelNumber];
        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations.
         */
        const SampleType** getArrayOfReadPointers() const {return const_cast<const SampleType**> (channels.data()); }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer.
         */
        SampleType** getArrayOfWritePointers() {return channels.data(); }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channels[c] + startOfRegion, channels[c] + endOfRegion, SampleType (0));
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
        int numChannelsAllocated;
        int numSamplesAllocated;
        int numSamplesUsed;

        juce::Array<SampleType*> channels;
        bool isInHostAccessMode = true;

        JUCE_LEAK_DETECTOR (CLSampleBufferReal)
    };

    template <typename SampleType>
    class CLSampleBufferComplex
    {
    public:

        CLSampleBufferComplex()
        {
            // not implemented yet
            jassertfalse;
        }

        /**
         * A simple way for templated functions to figure out if a buffer passed is complex valued. Can be completely
         * evaluated at compile time by an if constexpr statement
         */
        static constexpr bool isComplex() {return true; }

        /**
         * Enables access of the buffer for the host processor. This call might block until kernel executions in
         * progress on the CL device have been finished and might involve a memory copy from the CL device memory space
         * back to the host processors memory space in case of a non-shared memory architecture.
         */
        void enableHostDeviceAccess()
        {

        }

        /**
         * Enables access of the buffer for the host processor only if it is currently not used by any CL kernel.
         * Returns true if access could be enabled, false otherwise. This call might involve a memory copy from the CL
         * device memory space back to the host processors memory space in case of a non-shared memory architecture.
         */
        bool tryEnableHostDeviceAccess()
        {

        }

        /**
         * Disables access of the buffer for the host processor to pass it to a CL kernel.
         */
        void enableCLDeviceAccess()
        {

        }

        /**
         * Returns true if host device access is enabled, false if CL device access is enabled.
         */
        bool getCurrentAccessMode() const {return isInHostAccessMode; }

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

        }

        /**
         * Returns a pointer to the host memory buffer for a dedicated channel. Use this if you want to write to the
         * buffer, otherwise use getReadPointer. For speed reasons it does neither check if the channelNumber is inside
         * the valid range nor if host device access is enabled, so be careful when using it.
         */
        std::complex<SampleType>* getWritePointer (int channelNumber)
        {

        }

        /**
         * Returns a read-only array of pointers to the host memory buffers for all channels. Always use this for
         * read-only operations. For speed reasons it does not check if host device access is enabled, so be careful
         * when using it.
         */
        const std::complex<SampleType>** getArrayOfReadPointers() const
        {

        }

        /**
         * Returns an array of pointers to the host memory buffers for all channels. Use this if you want to write to
         * the buffer, otherwise use getReadPointer. For speed reasons it does not check if host device access is
         * enabled, so be careful when using it.
         */
        std::complex<SampleType>** getArrayOfWritePointers()
        {

        }

        /** Sets all samples in the region to zero. Passing -1 to endOfRegion leads to fill the buffer until its end */
        void clearBufferRegion (int startOfRegion = 0, int endOfRegion = -1)
        {
            if (endOfRegion == -1)
                endOfRegion = numSamplesAllocated;

            for (int c = 0; c < numChannelsAllocated; ++c)
                std::fill (channels[c] + startOfRegion, channels[c] + endOfRegion, SampleType (0));
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

        juce::Array<std::complex<SampleType>*> channels;
        bool isInHostAccessMode = true;

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
    };
}

#undef NTLAB_ASSERT_CHANNEL_AND_SAMPLE_COUNTS_PASSED_ARE_VALID
#undef NTLAB_OPERATION_ON_ALL_CHANNELS