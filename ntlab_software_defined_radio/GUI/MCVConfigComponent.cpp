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

#include "MCVConfigComponent.h"

namespace ntlab
{
    std::unique_ptr<juce::Component> MCVFileEngineManager::createEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints& constraints)
    {
        return std::unique_ptr<juce::Component> (new MCVConfigComponent (configurationInterface, constraints));
    }

    MCVConfigComponent::MCVConfigComponent (SDRIOEngineConfigurationInterface& configurationInterface, ntlab::SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints)
      : configInterface (configurationInterface),
        currentSettings (configInterface.getActiveConfig())
    {
        addAndMakeVisible (inFilePathLabel);
        addAndMakeVisible (outFilePathLabel);
        addAndMakeVisible (inFileBrowseButton);
        addAndMakeVisible (outFileBrowseButton);
        addAndMakeVisible (enableRxButton);
        addAndMakeVisible (enableTxButton);
        addAndMakeVisible (inputEndOfFileBehaviourBox);
        addAndMakeVisible (numOutChannelsLabel);
        addAndMakeVisible (applySettingsButton);

        inFilePathLabel   .setEditable (true);
        outFilePathLabel  .setEditable (true);
        numOutChannelsLabel.setEditable (true);

        inFilePathLabel    .setColour (juce::Label::ColourIds::backgroundColourId, juce::Colours::black);
        outFilePathLabel   .setColour (juce::Label::ColourIds::backgroundColourId, juce::Colours::black);
        numOutChannelsLabel.setColour (juce::Label::ColourIds::backgroundColourId, juce::Colours::black);

        inFilePathLabel    .setText (currentSettings.getProperty (MCVFileEngine::propertyInFile),         juce::NotificationType::dontSendNotification);
        outFilePathLabel   .setText (currentSettings.getProperty (MCVFileEngine::propertyOutFile),        juce::NotificationType::dontSendNotification);
        numOutChannelsLabel.setText (currentSettings.getProperty (MCVFileEngine::propertyNumOutChannels), juce::NotificationType::dontSendNotification);

        enableRxButton.setToggleState (currentSettings.getProperty (MCVFileEngine::propertyRxEnabled), juce::NotificationType::dontSendNotification);
        enableTxButton.setToggleState (currentSettings.getProperty (MCVFileEngine::propertyTxEnabled), juce::NotificationType::dontSendNotification);

        inputEndOfFileBehaviourBox.addItem ("Stop and fill buffer with zeros", 1);
        inputEndOfFileBehaviourBox.addItem ("Stop and resize last buffer",     2);
        inputEndOfFileBehaviourBox.addItem ("Loop input file endless",         3);

        auto endOfFileBehaviour = static_cast<MCVReader::EndOfFileBehaviour> (static_cast<int> (currentSettings.getProperty (MCVFileEngine::propertyInputEndOfFileBehaviour)));
        inputEndOfFileBehaviourBox.setSelectedItemIndex (endOfFileBehaviour + 1, juce::NotificationType::dontSendNotification);

        inFilePathLabel.onTextChange = [this]()
        {
            juce::File inFile (inFilePathLabel.getText());

            //if (inFile.)
        };

        applySettingsButton.onClick = [this]()
        {
            auto res = configInterface.setConfig (currentSettings);
            if (res.failed ())
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "Could not apply settings", res.getErrorMessage ());
            else
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::InfoIcon, "Successfully applied settings", "The MCV File Engine is ready for streaming.");
        };

        setSize (500, 300);
    }

    void MCVConfigComponent::paint (juce::Graphics& g)
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
        g.setColour (juce::Colours::white);

        auto bounds = getLocalBounds();

        bounds.removeFromLeft (5);
        bounds.removeFromTop  (10);
        auto inFileRow = bounds .removeFromTop (30);
        bounds.removeFromTop (40);
        auto outFileRow = bounds.removeFromTop (30);
        bounds.removeFromTop (40);
        auto thirdRow = bounds  .removeFromTop (30);

        g.drawText ("Input File", inFileRow.removeFromLeft (340), juce::Justification::topLeft);
        g.drawText ("Enable Rx",  inFileRow,                      juce::Justification::topLeft);

        g.drawText ("Output File", outFileRow.removeFromLeft (340), juce::Justification::topLeft);
        g.drawText ("Enable Tx",   outFileRow,                      juce::Justification::topLeft);

        g.drawText ("End of input file behaviour", thirdRow.removeFromLeft (205), juce::Justification::topLeft);
        g.drawText ("Num output channels",         thirdRow,                      juce::Justification::topLeft);
    }

    void MCVConfigComponent::resized ()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromLeft (5);
        bounds.removeFromTop (40);
        auto inFileRow  = bounds.removeFromTop (30);
        bounds.removeFromTop (40);
        auto outFileRow = bounds.removeFromTop (30);
        bounds.removeFromTop (40);
        auto thirdRow   = bounds.removeFromTop (30);
        bounds.removeFromTop (10);

        inFilePathLabel   .setBounds (inFileRow.removeFromLeft (300));
        inFileRow.removeFromLeft (5);
        inFileBrowseButton.setBounds (inFileRow.removeFromLeft (30));
        inFileRow.removeFromLeft (5);
        enableRxButton    .setBounds (inFileRow.removeFromLeft (30));

        outFilePathLabel   .setBounds (outFileRow.removeFromLeft (300));
        outFileRow.removeFromLeft (5);
        outFileBrowseButton.setBounds (outFileRow.removeFromLeft (30));
        outFileRow.removeFromLeft (5);
        enableTxButton     .setBounds (outFileRow.removeFromLeft (30));

        inputEndOfFileBehaviourBox.setBounds (thirdRow.removeFromLeft (200));
        thirdRow.removeFromLeft (5);
        numOutChannelsLabel       .setBounds (thirdRow.removeFromLeft (55));

        applySettingsButton.setBounds (bounds.removeFromTop (30).removeFromLeft (100));
    }

    juce::Result MCVConfigComponent::applyCurrentSettings()
    {
        // todo: do something here :D
		juce::ValueTree settingsToApply;
		return juce::Result::fail ("Not implemented yet");
    }
}
