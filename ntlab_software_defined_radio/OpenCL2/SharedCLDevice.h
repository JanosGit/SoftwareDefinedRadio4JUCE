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

#include "ntlab_OpenCLHelpers.h"
#include "clException.h"
#include <juce_core/juce_core.h>

namespace ntlab
{
    class SharedCLDevice
    {
    public:
        SharedCLDevice()
        {
#ifdef NTLAB_CL_FPGA_BINARY_FILE
            const juce::File fpgaBinaryFile (juce::File::getSpecialLocation (juce::File::SpecialLocationType::currentExecutableFile).getSiblingFile (NTLAB_CL_FPGA_BINARY_FILE))
            setUpFPGADevice (fpgaBinaryFile);
#else

    #ifdef NTLAB_PREFERRED_CL_DEVICE_TYPE
            cl_device_type preferredDeviceType = NTLAB_PREFERRED_CL_DEVICE_TYPE;
    #else
            cl_device_type preferredDeviceType = CL_DEVICE_TYPE_GPU;
    #endif
            getDeviceOnDefaultPlatform (preferredDeviceType);
#endif
            setupContextAndQueue();
        }

        ~SharedCLDevice() { clearSingletonInstance(); }

        bool isValid() { return queue() != nullptr; }

        juce::String getPlatformName() const { return platform.getInfo<CL_PLATFORM_NAME>(); }

        juce::String getDeviceName() const { return device.getInfo<CL_DEVICE_NAME>(); }

        const cl::Platform& getPlatform() const { return platform; }

        const cl::Device& getDevice() const { return device; }

        const cl::Context& getContext() const { return context; }

        const cl::CommandQueue& getCommandQueue() const { return queue; }

        const cl::Program& getFPGABinaryProgram() const { return binaryProgram; }

        cl::Program createProgramForDevice (const std::string& programSources, bool build = true) const
        {
            cl_int err;
            cl::Program program (context, programSources, build, &err);

            if (err != CL_SUCCESS)
                throw CLException ("Error creating program from sources", err);

            return program;
        }

        JUCE_DECLARE_SINGLETON (SharedCLDevice, true)

    private:
        cl::Platform     platform;
        cl::Device       device;
        cl::Context      context;
        cl::CommandQueue queue;

        cl::Program      binaryProgram;

        void getDeviceOnDefaultPlatform (cl_device_type deviceTypeToLookFor, bool useFallbackIfTypeNotAvailable = true)
        {
            cl_int err;
            platform = cl::Platform::getDefault (&err);
            if (err != CL_SUCCESS)
                throw CLException ("Error getting default platform: ", err);

            std::vector<cl::Device> allDevices;
            err = platform.getDevices (deviceTypeToLookFor, &allDevices);
            if (err != CL_SUCCESS)
            {
                juce::String errDescription = "Error getting ";

                switch (deviceTypeToLookFor)
                {
                    case CL_DEVICE_TYPE_ACCELERATOR:
                        errDescription += "accelerator or FPGA";
                        break;
                    case CL_DEVICE_TYPE_CPU:
                        errDescription += "CPU";
                        break;
                    case CL_DEVICE_TYPE_CUSTOM:
                        errDescription += "custom";
                        break;
                    case CL_DEVICE_TYPE_GPU:
                        errDescription += "GPU";
                        break;
                    default:
                        // seems you passed in an invalid device type?
                        jassertfalse;
                        errDescription += "INVALID";
                        break;
                }

                errDescription += " device";

                if (useFallbackIfTypeNotAvailable)
                    juce::Logger::writeToLog (errDescription + ": " + OpenCLHelpers::getErrorString (err) + ". Looking for default device instead");
                else
                    throw CLException (errDescription, err);
            }

            if (allDevices.size() > 0)
            {
                device = allDevices[0];
            }
            else
            {
                device = cl::Device::getDefault(&err);
                if (err != CL_SUCCESS)
                    throw CLException ("Error getting default device: ", err);
            }
        }

        void setUpFPGADevice (const juce::File& fpgaBinaryFile)
        {
            if (!fpgaBinaryFile.existsAsFile () || !fpgaBinaryFile.hasFileExtension ("aocx"))
                throw CLException ("Invalid fpga binary file " + fpgaBinaryFile.getFullPathName ());

            getDeviceOnDefaultPlatform (CL_DEVICE_TYPE_ACCELERATOR, false);

            juce::FileInputStream fpgaBinaryFileStream (fpgaBinaryFile);

            if (fpgaBinaryFileStream.openedOk ())
            {
                std::vector<unsigned char> fpgaBinaryFileContent (static_cast<size_t> (fpgaBinaryFileStream.getTotalLength ()));
                fpgaBinaryFileStream.read (fpgaBinaryFileContent.data (), static_cast<int> (fpgaBinaryFileContent.size ()));

                std::vector<cl::Device> fpgaDevice (1, device);
                cl::Program::Binaries fpgaBinaries;

                fpgaBinaries.emplace_back (fpgaBinaryFileContent);

                std::vector<cl_int> binaryStatus;
                cl_int err;

                binaryProgram = cl::Program (context, fpgaDevice, fpgaBinaries, &binaryStatus, &err);

                if (err != CL_SUCCESS)
                    throw CLException ("Error loading FPGA binaries from " + fpgaBinaryFile.getFullPathName (), err);

                if (binaryStatus[0] == CL_INVALID_BINARY)
                    throw CLException ("Error loading FPGA binaries from " + fpgaBinaryFile.getFullPathName () + " Binary does not match FPGA device");

                err = binaryProgram.build ();

                if (err != CL_SUCCESS)
                    throw CLException ("Error building FPGA program from " + fpgaBinaryFile.getFullPathName (), err);
            }
        }

        void setupContextAndQueue()
        {
            cl_int err;

            context  = cl::Context (device, nullptr, nullptr, nullptr, &err);
            if (err != CL_SUCCESS)
                throw CLException ("Error creating CL context: ", err);

            queue    = cl::CommandQueue (context, 0, &err);
            if (err != CL_SUCCESS)
                throw CLException ("Error creating CL CommandQueue: ", err);

            juce::Logger::writeToLog ("Using CL platform " + getPlatformName() + ", device " + getDeviceName());
        }
    };
}