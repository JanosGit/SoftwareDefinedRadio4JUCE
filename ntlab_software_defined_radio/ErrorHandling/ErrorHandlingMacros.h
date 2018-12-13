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


/*
 A collection of macros useful for error handling in functions, mainly to avoid a lot
 of boilerplate code. As macros should be avoided in modern C++ at all cost, this header
 is not included per default but only in a per source-file level in compile units where
 they are needed
*/

#pragma once

#include "boost/current_function.hpp"

/**
 * To be used in member functions that return a juce::Result. The class it is used in must also supply a member function
 * named errorDescription that takes some arbitrary error value (most probably an int return code which is 0 in case of
 * no error or an object with an operator bool that evaluates to false in case of no error) as argument and returns a
 * string clearly describing the error.
 */
#define NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR(error) \
    if (error) \
        return juce::Result::fail ("Error executing " + juce::String (BOOST_CURRENT_FUNCTION) + ": " + errorDescription (error))

/** Does the same as NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR but invokes some action before returning */
#define NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE(error, actionsToInvoke) \
    if (error) \
    { \
        actionsToInvoke\
        return juce::Result::fail ("Error executing " + juce::String (BOOST_CURRENT_FUNCTION) + ": " + errorDescription (error)); \
    } \


/** Prints an error in debug builds and invokes some user supplied actions afterwards */
#define NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS(error, actionsToInvoke) \
    if (error) \
    { \
        DBG ("Error executing " << juce::String (BOOST_CURRENT_FUNCTION) << ": " << errorDescription (error) << ". Continuing..."); \
        actionsToInvoke \
    }

/** Returns false if the condition evaluates to true, continues otherwise */
#define NTLAB_RETURN_FALSE_IF(condition) if (condition) return false;

/** Returns -1.0 if the condition evaluates to true, continues otherwise */
#define NTLAB_RETURN_MINUS_ONE_IF(condition) if (condition) return -1.0;

/**
 * Returns false and prints some Debug information in debug builds if the juce::Result passed in evaluates to be failed.
 * Returns true otherwise.
 */
#define NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE(result) \
    if (result.failed()) \
    { \
        DBG ("Error executing " << juce::String (BOOST_CURRENT_FUNCTION) << result.getErrorMessage() << ". Continuing..."); \
        return false; \
   } \
   return true

/**
 * Returns -1.0 and prints some Debug information in debug builds if the error value passed evaluates to true. Continues
 * otherwise.
 */
#define NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED(error) \
    if (error) \
    { \
        DBG ("Error executing " << juce::String (BOOST_CURRENT_FUNCTION) << ": " << errorDescription (error) << ". Continuing..."); \
        return -1.0; \
    }

/**
* Returns -1.0 and prints some Debug information in debug builds if the error value passed evaluates to true. Returns
* the return value passed to the function otherwise.
*/
#define NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE(error, returnValue) \
    if (error) \
    { \
        DBG ("Error executing " << juce::String (BOOST_CURRENT_FUNCTION) << ": " << errorDescription (error) << ". Continuing..."); \
        returnValue = -1.0; \
    } \
    return returnValue