

#pragma once

#include <iostream>
#include <fstream>
#include <algorithm>

#ifdef WIN32
#include <Windows.h>
#elif defined(__APPLE__) || defined(__MACOSX)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

#include "cl2WithVersionChecks.h"


namespace ntlab
{
    namespace OpenCLHelpers
    {

        /** Extracts all names from a vector of e.g. Platform or Device objects and optionally prints them */
        template <cl_int nameType, typename T>
        static std::vector<std::string> getAllNames (std::vector<T>& vec, bool print = false)
        {
            std::vector<std::string> results;
            results.reserve (vec.size());

            for (T& v : vec)
            {
                std::string platformName (v.template getInfo<nameType>().c_str());

                if (print)
                    std::cout << "  " << platformName << std::endl;

                results.emplace_back (platformName);
            }

            return results;
        }

        /**
         * Returns the platform object matching the platfom name to look for or an invalid platform object if none could
         * be found. Check the object returned with isValid. Optionally prints out all available platforms found.
         */
        static cl::Platform getPlatformIfAvailable (const std::string& platformNameToLookFor, bool printAllAvailablePlatforms = false)
        {
            std::vector<cl::Platform> allPlatforms;
            auto err = cl::Platform::get (&allPlatforms);

            if (err != CL_SUCCESS)
                return cl::Platform();

            if (printAllAvailablePlatforms)
                std::cout << "Available platforms:\n";

            auto allPlatformNames = getAllNames<CL_PLATFORM_NAME> (allPlatforms, printAllAvailablePlatforms);

            for (int i = 0; i < allPlatforms.size(); ++i)
            {
                if (allPlatformNames[i] == platformNameToLookFor)
                    return allPlatforms[i];
            }

            return cl::Platform();
        }

        /**
         * Returns the device object matching the device name to look for or an invalid device object if none could
         * be found. Check the object returned with isValid. Optionally prints out all available devices found.
         */
        static cl::Device getDeviceIfAvailable (const cl::Platform& platformToUse, const std::string& deviceNameToLookFor, bool printAllAvailableDevices = false)
        {
            std::vector<cl::Device> allDevices;
            auto err = platformToUse.getDevices (CL_DEVICE_TYPE_ALL, &allDevices);

            if ((err != CL_SUCCESS) || (allDevices.size() == 0))
                return cl::Device();

            if (printAllAvailableDevices)
                std::cout << "Available devices:\n";

            auto allDeviceNames = getAllNames<CL_DEVICE_NAME> (allDevices, printAllAvailableDevices);

            for (int i = 0; i < allDevices.size(); ++i)
            {
                if (allDeviceNames[i] == deviceNameToLookFor)
                    return allDevices[i];
            }

            return cl::Device();
        }

        /*
         * Especially useful to refer to kernel source- or binary files in a portable way. Put in the path of the kernel
         * file relative to the host executable binary location and the function will return an absolute path to the
         * kernel file. This will work reliabily in every situation while passing a relative path to the file directly
         * to the cl funtions might fail under some circumstances as the current working directory at runtime is not
         * neccessarily the location of the exectuable.
         *
         * Furthermore if the relative path passed in contains / chars it will automatically be converted to \ on
         * Windows making the code even more portable.
         *
         * If for some reasons the call fails an empty string will be returned.
         */
        static std::string getAbsolutePathFromPathRelativeToExecutable (const std::string& pathRelativeToExecutable)
        {
            char* pathToCurrentExecutable = nullptr;

#ifdef _WIN32
            char pathBuffer[MAX_PATH];
            DWORD pathLength = GetModuleFileNameA (NULL, pathBuffer, MAX_PATH);
            if (pathLength == 0)
                return "";

            pathToCurrentExecutable = pathBuffer;
#elif defined(__APPLE__) || defined(__MACOSX)

            char executablePathBuffer[PATH_MAX];
            uint32_t pathLength = PATH_MAX;

            int error = _NSGetExecutablePath (executablePathBuffer, &pathLength);
            if (error != 0)
                return "";

            pathToCurrentExecutable = executablePathBuffer;
#else // Linux

            char executablePathBuffer[PATH_MAX];
            ssize_t pathLength = readlink ("/proc/self/exe", executablePathBuffer, PATH_MAX - 1);
            if (pathLength == -1)
                return "";

            executablePathBuffer[pathLength] = '\0';
            pathToCurrentExecutable = executablePathBuffer;
#endif

            // find last / or \ character
            char* endOfPath = pathToCurrentExecutable + strlen (pathToCurrentExecutable);
            for (char* p = endOfPath; p != pathToCurrentExecutable; --p)
            {
                char previousChar = *(p - 1);
#ifdef _WIN32
                if (previousChar == '\\')
#else
                if (previousChar == '/')
#endif
                {
                    *p = '\0';
                    break;
                }
            }

            std::string pathToReturn = pathToCurrentExecutable + pathRelativeToExecutable;

#ifdef _WIN32
            std::replace (pathToReturn.begin(), pathToReturn.end(), '/', '\\');
            return pathToReturn;
#else
            char realExecutablePath[PATH_MAX];
            if (realpath (pathToReturn.c_str(), realExecutablePath) == NULL)
            {
                char* err = strerror (errno);
                std::cerr << "Error while resolving absolute path to \"" << pathRelativeToExecutable << "\": " << err << std::endl;
                return "";
            }

            return realExecutablePath;
#endif
        }

        /** Returns true if the platform or device object passed in refers to a valid platform */
        template <typename T>
        static bool isValid (const cl::detail::Wrapper<T>& p) { return p() != NULL; }

        /**
         * Returns a string representation of all CL error codes.
         * Found here: https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
         */
        static const juce::String getErrorString (cl_int error)
        {
            switch(error)
            {
                // run-time and JIT compiler errors
                case 0: return "CL_SUCCESS";
                case -1: return "CL_DEVICE_NOT_FOUND";
                case -2: return "CL_DEVICE_NOT_AVAILABLE";
                case -3: return "CL_COMPILER_NOT_AVAILABLE";
                case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
                case -5: return "CL_OUT_OF_RESOURCES";
                case -6: return "CL_OUT_OF_HOST_MEMORY";
                case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
                case -8: return "CL_MEM_COPY_OVERLAP";
                case -9: return "CL_IMAGE_FORMAT_MISMATCH";
                case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
                case -11: return "CL_BUILD_PROGRAM_FAILURE";
                case -12: return "CL_MAP_FAILURE";
                case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
                case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
                case -15: return "CL_COMPILE_PROGRAM_FAILURE";
                case -16: return "CL_LINKER_NOT_AVAILABLE";
                case -17: return "CL_LINK_PROGRAM_FAILURE";
                case -18: return "CL_DEVICE_PARTITION_FAILED";
                case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

                    // compile-time errors
                case -30: return "CL_INVALID_VALUE";
                case -31: return "CL_INVALID_DEVICE_TYPE";
                case -32: return "CL_INVALID_PLATFORM";
                case -33: return "CL_INVALID_DEVICE";
                case -34: return "CL_INVALID_CONTEXT";
                case -35: return "CL_INVALID_QUEUE_PROPERTIES";
                case -36: return "CL_INVALID_COMMAND_QUEUE";
                case -37: return "CL_INVALID_HOST_PTR";
                case -38: return "CL_INVALID_MEM_OBJECT";
                case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
                case -40: return "CL_INVALID_IMAGE_SIZE";
                case -41: return "CL_INVALID_SAMPLER";
                case -42: return "CL_INVALID_BINARY";
                case -43: return "CL_INVALID_BUILD_OPTIONS";
                case -44: return "CL_INVALID_PROGRAM";
                case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
                case -46: return "CL_INVALID_KERNEL_NAME";
                case -47: return "CL_INVALID_KERNEL_DEFINITION";
                case -48: return "CL_INVALID_KERNEL";
                case -49: return "CL_INVALID_ARG_INDEX";
                case -50: return "CL_INVALID_ARG_VALUE";
                case -51: return "CL_INVALID_ARG_SIZE";
                case -52: return "CL_INVALID_KERNEL_ARGS";
                case -53: return "CL_INVALID_WORK_DIMENSION";
                case -54: return "CL_INVALID_WORK_GROUP_SIZE";
                case -55: return "CL_INVALID_WORK_ITEM_SIZE";
                case -56: return "CL_INVALID_GLOBAL_OFFSET";
                case -57: return "CL_INVALID_EVENT_WAIT_LIST";
                case -58: return "CL_INVALID_EVENT";
                case -59: return "CL_INVALID_OPERATION";
                case -60: return "CL_INVALID_GL_OBJECT";
                case -61: return "CL_INVALID_BUFFER_SIZE";
                case -62: return "CL_INVALID_MIP_LEVEL";
                case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
                case -64: return "CL_INVALID_PROPERTY";
                case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
                case -66: return "CL_INVALID_COMPILER_OPTIONS";
                case -67: return "CL_INVALID_LINKER_OPTIONS";
                case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

                    // extension errors
                case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
                case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
                case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
                case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
                case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
                case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
                default: return "Unknown OpenCL error";
            }
        }


        static const juce::String getEventCommandExecutionStatusString (cl_int status)
        {
            if (status < 0)
                return getErrorString (status);

            switch (status)
            {
                case CL_QUEUED:    return "CL_QUEUED";
                case CL_SUBMITTED: return "CL_SUBMITTED";
                case CL_RUNNING:   return "CL_RUNNING";
                case CL_COMPLETE:  return "CL_COMPLETE";

                default: return "Unknown CL event command execution status";
            }
        }

    };
}
