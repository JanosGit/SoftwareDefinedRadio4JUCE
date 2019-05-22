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

#include "cl2WithVersionChecks.h"
#include "clException.h"

namespace ntlab
{
    /**
     * A simple fixed-size Array type, that uses a cl::Buffer as underlying storage type and allows mapping/unmapping
     * operations. Note that trying to access elements in an unmapped state will lead to a runtime error
     * @tparam T
     */
    template <typename T>
    class CLArray
    {
    public:
        /**
         * Creates a fixed-sized array, using CL-storage under the hood. You need to specify a cl::Context and
         * cl::CommandQueue that should be used for creation and mapping/unmapping operations. The memFlags allow
         * you to specify how the array should be used from the kernel side, default is CL_MEM_READ_WRITE, however
         * adjusting it to the use-case can lead to performance enhancements.
         */
        CLArray (const int numElements, cl::Context& contextToUse, cl::CommandQueue& queueToUse, cl_mem_flags memFlags = CL_MEM_READ_WRITE)
        : sizeInBytes (numElements * sizeof (T)),
          queue  (queueToUse)
        {
			cl_int err;
			buffer = cl::Buffer (contextToUse, CL_MEM_ALLOC_HOST_PTR | memFlags, sizeInBytes, nullptr, &err);

			if (err != CL_SUCCESS)
				throw CLException ("CL error while creating cl::Buffer object of size" + juce::String (sizeInBytes) + " bytes", err);

			map (CL_TRUE);
        }

        T& operator[] (int index) const
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            return ptr[index];
        }

        /** Set the value of a single element in the array */
        inline void set (int index, T newValue) const
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            ptr[index] = newValue;
        }

        /** For stl and range-based-loop compatibility */
        inline T* begin() const noexcept
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            return ptr;
        }

        /** For stl and range-based-loop compatibility */
        inline T* end() const noexcept
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            return dataEnd;
        }

        /** For stl and range-based-loop compatibility */
        inline T* data() const noexcept
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            return ptr;
        }

        /** For juce::Array compatibility */
        inline T getUnchecked (int index) const
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            return ptr[index];
        }

        /** Fills all elements in the array with the same value */
        void fill (T valueToFillIn)
        {
            // You are trying to access the array in an unmapped state
            jassert (ptr != nullptr);
            for (T* element = ptr; element != dataEnd; ++element)
                *element = valueToFillIn;
        }

        /**
         * Maps the array to the host memory space, using the CommandQueue provided by the constructor. Per default
         * it performs a non-blocking map and maps the memory for writing, you can override those defaults to your needs
         */
        T* map (cl_bool blockingMap = CL_FALSE, cl_map_flags mapFlags = CL_MAP_WRITE)
        {
            if (!isMapped())
            {
				cl_int err;
				ptr = static_cast<T*> (queue.enqueueMapBuffer (buffer, blockingMap, mapFlags, 0, sizeInBytes, nullptr, nullptr, &err));
				if (err != CL_SUCCESS)
				{
					ptr = nullptr;
					return ptr;
				}
                dataEnd = juce::addBytesToPointer (ptr, sizeInBytes);
            }

            return ptr;
        }

        /**
         * Unmaps the array from the host memory space to make it accessible to the device, using the CommandQueue
         * provided by the constructor. This is a non-blocking call, so make sure to wait for the queue to have finished
         * before using the memory in the kernel.
         */
        cl::Buffer& unmap()
        {
            if (isMapped())
            {
                queue.enqueueUnmapMemObject (buffer, ptr);
                ptr     = nullptr;
                dataEnd = nullptr;
            }

            return buffer;
        }

        bool isMapped() const { return ptr != nullptr; }

    private:
        const size_t sizeInBytes;
        cl::Buffer   buffer;
        T*           ptr     = nullptr;
        T*           dataEnd = nullptr;

        cl::CommandQueue& queue;
    };
}

