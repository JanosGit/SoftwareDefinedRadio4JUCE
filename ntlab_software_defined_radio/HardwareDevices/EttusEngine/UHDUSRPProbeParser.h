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
#include "UHDReplacement.h"

namespace ntlab
{
    /**
 * Calls uhd_usrp_probe as a child process and tries to parse the output to a var structure that
 * can be stored and sent as a json structure. In case of an error, an empty var will be returned.
 */
    juce::var parseUHDUSRPProbe (ntlab::UHDr::Ptr uhd, juce::StringArray* originalOutput = nullptr);
}



