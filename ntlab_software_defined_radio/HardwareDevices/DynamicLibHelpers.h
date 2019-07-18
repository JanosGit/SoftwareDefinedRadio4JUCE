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

// a helper class that takes care of closing the library and generating an automated error message containing
// the function name that couldn't be loaded

template <typename LibHolderClassName>
class LoadingError {
public:
    LoadingError (juce::String &functionName, juce::String &result) :
            fnName (functionName),
            res (result) {};

    // this is called if loading the most recent function failed.
    LibHolderClassName *lastFunction() {
        res = "Error loading function " + fnName;
        return nullptr;
    }

private:
    juce::String &fnName;
    juce::String &res;
};

// A macro to avoid a lot of repetitive boilerplate code
#define NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS(instance, fptr, fnName) instance->fptr = (decltype (instance->fptr)) lib.getFunction (functionName = fnName); \
                                                                          if (instance->fptr == nullptr) \
                                                                              return loadingError.lastFunction();