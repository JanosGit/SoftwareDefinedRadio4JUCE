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

#include "../JuceLibraryCode/JuceHeader.h"

class SimpleUnitTestRunner : public UnitTestRunner
{
    void logMessage (const String& message) override
    {
        std::cout << message << std::endl;
    }
};

//==============================================================================
class UnitTestApplication : public juce::JUCEApplicationBase
{
public:
    const juce::String getApplicationName()    override {return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override {return ProjectInfo::versionString; }

    bool moreThanOneInstanceAllowed() override { return false; };
    void initialise (const juce::String& commandLine) override
    {
        auto commands = juce::StringArray::fromTokens (commandLine, true);
        if (commands.size() == 1)
            runner.runAllTests();
        else
        {
            int64_t seed = commands[1].getHexValue64();
            runner.runAllTests (seed);
        }

        for (int i = 0; i < runner.getNumResults(); ++i)
        {
            if (runner.getResult(i)->failures > 0)
                setApplicationReturnValue (1);
            else
                setApplicationReturnValue (0);
        }

        quit();
    }

    void systemRequestedQuit()                        override { quit(); }
    void shutdown()                                   override {}
    void anotherInstanceStarted (const juce::String&) override {}
    void suspended()                                  override {}
    void resumed()                                    override {}
    
    void unhandledException (const std::exception* exception, const juce::String& sourceFileName, int lineNumber) override
    {
        std::cerr << "Unhandled exception from " << sourceFileName << " line " << lineNumber << ":\n" << exception->what() << std::endl;
    }

private:
    SimpleUnitTestRunner runner;
};

START_JUCE_APPLICATION (UnitTestApplication);