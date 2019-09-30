#pragma once

#include <exception>
#include <juce_core/juce_core.h>
#include "ntlab_OpenCLHelpers.h"

namespace ntlab
{
	class CLException : public std::exception
	{
	public:
		/** Creates an exception that will contain the message "CL Error: CL_SOMEERRORCODE" where CL_SOMEERRORCODE is the interpretaion of the error code passe in. */
		CLException (cl_int errorCode) : message ("CL Error: " + OpenCLHelpers::getErrorString (errorCode)) {};

		/** Creates an exception where the description string is placed before the error code interpretation. */
		CLException (const juce::String& description, cl_int errorCode) : message (description + ": " + OpenCLHelpers::getErrorString(errorCode)) {};


		CLException (cl_int errorCode, const std::pair<cl::Device, std::string>& deviceBuildLog)
		: message ("Error building CL program: " + OpenCLHelpers::getErrorString (errorCode) +
		           ". Build log for device "     + deviceBuildLog.first.getInfo<CL_DEVICE_NAME>() +
		           ":\n"                         + deviceBuildLog.second) {};

		/** Creates an exception just containig the description. */
		CLException (const juce::String& description) : message (description) {};

		const char* what() const noexcept override
		{
			return message.toRawUTF8();
		}

	private:
		const juce::String message;

	};
}

