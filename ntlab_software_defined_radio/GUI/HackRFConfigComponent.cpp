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

#include "HackRFConfigComponent.h"
#include "../HardwareDevices/HackRFEngine/HackRfEngine.h"

namespace ntlab
{
    std::unique_ptr<juce::Component> HackRFEngineManager::createEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints& constraints)
    {
        return std::unique_ptr<juce::Component> (new HackRFConfigComponent (configurationInterface, constraints));
    }

    HackRFConfigComponent::HackRFConfigComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints)
     : configInterface   (configurationInterface),
       configConstraints (constraints),
       currentSettings   (configInterface.getActiveConfig())
    {
        using Constr = SDRIOEngineConfigurationInterface::ConfigurationConstraints;

        addAndMakeVisible (deviceSelectionBox);
        addAndMakeVisible (sampleRateLabel);
        addAndMakeVisible (centerFreqLabel);
        addAndMakeVisible (applySettingsButton);

        sampleRateLabel.setEditable (true);
        centerFreqLabel.setEditable (true);

        lastSampleRate = currentSettings.getProperty (HackRFEngine::propertySampleRate);
        lastCenterFreq = currentSettings.getProperty (HackRFEngine::propertyCenterFrequency);

        sampleRateLabel.setText (lastSampleRate, juce::NotificationType::dontSendNotification);
        centerFreqLabel.setText (lastCenterFreq, juce::NotificationType::dontSendNotification);

        auto tree = configurationInterface.getDeviceTree();

        auto* deviceListVarArray = tree[HackRFEngine::propertyDeviceList].getArray();
        juce::StringArray deviceListStringArray;

        for (auto& device : *deviceListVarArray)
            deviceListStringArray.add (device);

        deviceSelectionBox.addItemList (deviceListStringArray, 1);

        deviceSelectionBox.onChange = [this] ()
        {
            currentSettings.setProperty (HackRFEngine::propertyDeviceName, deviceSelectionBox.getText(), nullptr);
        };

        sampleRateLabel.onTextChange = [this] ()
        {
            auto newSampleRate            = sampleRateLabel.getText();
            auto newSampleRateDoubleValue = newSampleRate.getDoubleValue();

            if (newSampleRate.containsOnly (".0123456789") && configConstraints.isValidValue (Constr::sampleRate, newSampleRateDoubleValue))
            {
                currentSettings.setProperty (HackRFEngine::propertySampleRate, newSampleRateDoubleValue, nullptr);
                lastSampleRate = newSampleRate;
            }
            else
                sampleRateLabel.setText (lastSampleRate, juce::NotificationType::dontSendNotification);
        };

        centerFreqLabel.onTextChange = [this] ()
        {
            auto newCenterFreq            = centerFreqLabel.getText();
            auto newCenterFreqDoubleValue = newCenterFreq.getDoubleValue();

            if (newCenterFreq.containsOnly (".0123456789") && configConstraints.isValidValue (Constr::sampleRate, newCenterFreqDoubleValue))
            {
                currentSettings.setProperty (HackRFEngine::propertyCenterFrequency, newCenterFreqDoubleValue, nullptr);
                lastCenterFreq = newCenterFreq;
            }
            else
                centerFreqLabel.setText (lastCenterFreq, juce::NotificationType::dontSendNotification);
        };

        applySettingsButton.onClick = [this] ()
        {
            auto res = configInterface.setConfig (currentSettings);
            if (res.failed ())
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "Could not apply settings", res.getErrorMessage ());
            else
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::InfoIcon, "Successfully applied settings", "The HackRF Engine is ready for streaming.");
        };

        setSize (500, 300);
    }

    void HackRFConfigComponent::paint (juce::Graphics& g)
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    }

    void HackRFConfigComponent::resized ()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromLeft (5);

        deviceSelectionBox.setBounds (bounds.removeFromTop (20));
        sampleRateLabel.setBounds (bounds.removeFromTop (20));
        centerFreqLabel.setBounds (bounds.removeFromTop (20));
        applySettingsButton.setBounds (bounds.removeFromTop (30));
    }
}