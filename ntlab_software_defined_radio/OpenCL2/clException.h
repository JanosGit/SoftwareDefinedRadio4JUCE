#pragma once

#include <exception>
#include <juce_core/juce_core.h>
#include "ntlab_OpenCLHelpers.h"

namespace ntlab
{
	class CLException : public std::exception
	{
	public:
		/**
		 * Creates an exception that will contain the message "CL Error: CL_SOMEERRORCODE" where CL_SOMEERRORCODE is the interpretaion of the error code passe in.
		 */
		CLException (cl_int errorCode) : message ("CL Error: " + juce::String (OpenCLHelpers::getErrorString (errorCode))) {};

		/** 
		 * Creates an exception where the description string is placed before the error code interpretation.
		 */
		CLException (const juce::String& description, cl_int errorCode) : message (description + ": " + juce::String(OpenCLHelpers::getErrorString(errorCode))) {};

		const char* what() const noexcept override
		{
			return message.toRawUTF8();
		}

	private:
		const juce::String message;

	};
}

