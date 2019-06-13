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
#include <juce_gui_basics/juce_gui_basics.h>

namespace ntlab
{
    class MCVConfigComponent : public juce::Component
    {
    public:
        MCVConfigComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints);

        void paint (juce::Graphics& g) override;

        void resized() override;

    private:
        SDRIOEngineConfigurationInterface& configInterface;
        juce::ValueTree currentSettings;

        juce::Label        inFilePathLabel;
        juce::Label        outFilePathLabel;
        juce::TextButton   inFileBrowseButton  {"..."};
        juce::TextButton   outFileBrowseButton {"..."};
        juce::ToggleButton enableRxButton;
        juce::ToggleButton enableTxButton;
        juce::ComboBox     inputEndOfFileBehaviourBox;
        juce::Label        numOutChannelsLabel;
        juce::TextButton   applySettingsButton {"Apply Settings"};

        juce::Result applyCurrentSettings();
    };
}