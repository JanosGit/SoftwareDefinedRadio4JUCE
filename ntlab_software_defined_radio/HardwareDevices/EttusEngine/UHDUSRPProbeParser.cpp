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

#include "UHDUSRPProbeParser.h"

/**
 * Calls uhd_usrp_probe as a child process and tries to parse the output to a var structure that
 * can be stored and sent as a json structure. In case of an error, an empty var will be returned.
 */
juce::var parseUHDUSRPProbe (ntlab::UHDr::Ptr uhd)
{

    // find all devices
    juce::StringArray allIPAddresses;
    auto allDevices = uhd->findAllDevices ("");
    for (auto &d : allDevices)
        allIPAddresses.add (d.getValue ("addr", "0.0.0.0"));

    // prepare the tree structure
    juce::Array<juce::DynamicObject*> tree;
    tree.add (new juce::DynamicObject);

    juce::Identifier identifierID = "ID";
    juce::Identifier identifierMin = "min";
    juce::Identifier identifierMax = "max";
    juce::Identifier identifierStepWidth = "step width";
    juce::Identifier identifierUnit = "unit";

    juce::StringArray probeArgs ("uhd_usrp_probe");

    // iterate over all devices
    int currentDevice = 0;
    for (auto &a : allIPAddresses)
    {
        // go back to the trees root
        int lastIterationDepth = 0;
        tree.removeLast (tree.size() - 1);

        probeArgs.set (1, "--args=addr=" + a);

        juce::ChildProcess uhdUSRPProbe;
        bool success = uhdUSRPProbe.start (probeArgs);

        if (!success)
        {
            delete tree.getFirst();
            return juce::var();
        }

        juce::String outputToParse = uhdUSRPProbe.readAllProcessOutput();

        if (outputToParse.isEmpty())
        {
            delete tree.getFirst();
            return juce::var();
        }

        auto linesToParse = juce::StringArray::fromLines (outputToParse);

        int idxOfDeviceEntry = 0;
        for (auto &l : linesToParse)
        {
            if (l.contains ("Device: "))
                break;

            idxOfDeviceEntry++;
        }
        if (idxOfDeviceEntry < linesToParse.size())
        {
            linesToParse.set (idxOfDeviceEntry, linesToParse[idxOfDeviceEntry] + " " + juce::String (currentDevice));
            currentDevice++;
        }

        for (auto &line : linesToParse)
        {
            // remove whitespaces from the start
            line = line.trimStart();

            if (line.startsWith ("|") || line.startsWith ("/"))
            {
                // count "|" characters to get the depth
                int iterationDepth = 0;
                const char* length = line.toRawUTF8() + line.length();
                for (char *c = (char*)line.toRawUTF8(); c < length; ++c)
                {
                    if (*c == '|')
                    {
                        iterationDepth++;
                    }
                    else if (*c != ' ')
                    {
                        break;
                    }
                }

                // the end of a branch is reached
                if (iterationDepth < lastIterationDepth)
                {
                    tree.removeLast ((lastIterationDepth - iterationDepth) * 2);
                    lastIterationDepth = iterationDepth;
                }

                // clear up the line
                line = line.trimCharactersAtStart ("| _/");

                // just jump to the next line if it's empty now
                if (line.isEmpty())
                    continue;

                // create a pair of property and value
                auto valuePair = juce::StringArray::fromTokens (line, ":", "");

                // special case: mac-address has : chars
                if (valuePair[0].equalsIgnoreCase("mac-addr"))
                    valuePair.set (1, line.fromFirstOccurrenceOf ("mac-addr: ", false, true));

                // if the new depth is higher than the previous one, add a new Dynamic object
                if (iterationDepth > lastIterationDepth)
                {

                    // Rename "Device" to "USRP Device"
                    if (valuePair[0].equalsIgnoreCase ("Device"))
                        valuePair.set (0, "USRP " + valuePair[0]);

                    juce::Identifier newID = valuePair[0];
                    juce::DynamicObject *branchForNewID = nullptr;

                    if (tree.getLast()->hasProperty (newID))
                    {
                        branchForNewID = tree.getLast()->getProperty(newID).getDynamicObject();
                    }
                    else
                    {
                        branchForNewID = new juce::DynamicObject;
                        tree.getLast()->setProperty (newID, branchForNewID);
                    }

                    tree.add (branchForNewID);

                    juce::DynamicObject *newProperty = new juce::DynamicObject;
                    juce::Identifier secondNewID = valuePair[1].trimStart();
                    branchForNewID->setProperty (secondNewID, newProperty);
                    tree.add (newProperty);

                    lastIterationDepth = iterationDepth;
                }
                else
                {
                    // check if the value is an enumeration
                    if (valuePair[1].contains (","))
                    {
                        auto value = valuePair[1].removeCharacters(" ");
                        auto array = juce::StringArray::fromTokens (value, ",", "");
                        tree.getLast ()->setProperty (valuePair[0], array);
                    }

                        // check if it's a range
                    else if (valuePair[1].contains (" to "))
                    {
                        juce::String lowerLimit = valuePair[1].upToFirstOccurrenceOf (" to ", false, false);
                        juce::String remainigPart = valuePair[1].fromFirstOccurrenceOf (" to ", false, false);
                        juce::String upperLimit, step;
                        if (remainigPart.contains (" step "))
                        {
                            upperLimit = remainigPart.upToFirstOccurrenceOf (" step ", false, false);
                            remainigPart = remainigPart.fromFirstOccurrenceOf (" step ", false, false);
                            step = remainigPart.upToFirstOccurrenceOf (" ", false, false);
                        }
                        else
                            upperLimit = remainigPart.upToFirstOccurrenceOf (" ", false, false);

                        juce::String unit = remainigPart.fromFirstOccurrenceOf (" ", false, false);

                        auto range = new juce::DynamicObject;
                        range->setProperty (identifierMin, lowerLimit.getDoubleValue());
                        range->setProperty (identifierMax, upperLimit.getDoubleValue());
                        if (step.isNotEmpty())
                        {
                            range->setProperty (identifierStepWidth, step.getDoubleValue());
                        }
                        else
                            range->setProperty (identifierStepWidth, 0.0);

                        range->setProperty (identifierUnit, unit);

                        tree.getLast()->setProperty (valuePair[0], range);
                    }
                    else
                        tree.getLast()->setProperty (valuePair[0], valuePair[1].trimStart());
                }

            }
        }
    }

    return juce::var (tree.getFirst());
}

