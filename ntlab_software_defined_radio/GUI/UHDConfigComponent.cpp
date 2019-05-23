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

#include "UHDConfigComponent.h"

namespace ntlab
{
    std::unique_ptr<juce::Component> UHDEngineManager::createEngineConfigurationComponent (ntlab::SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints& constraints)
    {
        return std::unique_ptr<juce::Component> (new UHDEngineConfigurationComponent (configurationInterface, constraints));
    }

    const juce::Identifier UHDEngineConfigurationComponent::numRxChannels ("numRxChannels");
    const juce::Identifier UHDEngineConfigurationComponent::numTxChannels ("numTxChannels");
    const juce::Identifier UHDEngineConfigurationComponent::rxChannelsAssigned ("rxChannelsAssigned");
    const juce::Identifier UHDEngineConfigurationComponent::txChannelsAssigned ("txChannelsAssigned");
    const juce::Identifier UHDEngineConfigurationComponent::currentComboBoxID ("currentComboBoxID");

    UHDEngineConfigurationComponent::UHDEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints)
            : configInterface (configurationInterface),
              configConstraints (constraints)
    {
        deviceValueTree = configInterface.getDeviceTree ();

        // add some properties to the root for easier management
        deviceValueTree.setProperty (numRxChannels, 0, nullptr);
        deviceValueTree.setProperty (numTxChannels, 0, nullptr);
        deviceValueTree.setProperty (rxChannelsAssigned, juce::int64 (0), nullptr);
        deviceValueTree.setProperty (txChannelsAssigned, juce::int64 (0), nullptr);

        addAndMakeVisible (numRxChannelsLabel);
        addAndMakeVisible (numTxChannelsLabel);
        addAndMakeVisible (sampleRateLabel);
        addAndMakeVisible (syncSetupBox);
        addAndMakeVisible (applyChangesButton);
        addAndMakeVisible (treeView);

        using Constr = SDRIOEngineConfigurationInterface::ConfigurationConstraints;

        // use the range for both, checking the user constraints and the internal constraints
        if (!configConstraints.hasMaxValue (Constr::numRxChannels))
            configConstraints.setMax (Constr::numRxChannels, static_cast<int> (maxNumChannels));
        if (!configConstraints.hasMaxValue (Constr::numTxChannels))
            configConstraints.setMax (Constr::numTxChannels, static_cast<int> (maxNumChannels));

        // the Config component can only manage a limited amount of channels (should be 64 on all 64 Bit systems)
        jassert (configConstraints.getMaxInt (Constr::numRxChannels) < maxNumChannels);
        jassert (configConstraints.getMaxInt (Constr::numTxChannels) < maxNumChannels);

        constrainCenterFrequencyInTree (deviceValueTree);

        if (configConstraints.hasFixedValue (Constr::numRxChannels))
        {
            const int desiredNumRxChannels = configConstraints.getMinInt (Constr::numRxChannels);
            numRxChannelsLabel.setText (juce::String (desiredNumRxChannels), juce::NotificationType::dontSendNotification);
            deviceValueTree.setProperty (numRxChannels, desiredNumRxChannels, nullptr);
        }
        else
        {
            numRxChannelsLabel.setText (juce::String (configConstraints.clipToValidValue (Constr::numRxChannels, 0)), juce::NotificationType::dontSendNotification);
            numRxChannelsLabel.setEditable (true);
            numRxChannelsLabel.onTextChange = [this] ()
            {
                auto newNumChannels = numRxChannelsLabel.getText ();
                int newNumChannelsIntValue = newNumChannels.getIntValue ();

                if (newNumChannels.containsOnly ("0123456789") && configConstraints.isValidValue (Constr::numRxChannels, newNumChannelsIntValue))
                {
                    deviceValueTree.setProperty (numRxChannels, newNumChannelsIntValue, nullptr);
                    if (rootItem != nullptr)
                        rootItem->refreshSubItems ();
                }
                else
                    numRxChannelsLabel.setText (deviceValueTree.getProperty (numRxChannels), juce::NotificationType::dontSendNotification);
            };
        }

        if (configConstraints.hasFixedValue (Constr::numTxChannels))
        {
            const int desiredNumTxChannels = configConstraints.getMinInt (Constr::numTxChannels);
            numTxChannelsLabel.setText (juce::String (desiredNumTxChannels), juce::NotificationType::dontSendNotification);
            deviceValueTree.setProperty (numTxChannels, desiredNumTxChannels, nullptr);
        }
        else
        {
            numTxChannelsLabel.setText (juce::String (configConstraints.clipToValidValue (Constr::numTxChannels, 0)), juce::NotificationType::dontSendNotification);
            numTxChannelsLabel.setEditable (true);
            numTxChannelsLabel.onTextChange = [this] ()
            {
                auto newNumChannels = numTxChannelsLabel.getText ();
                int newNumChannelsIntValue = newNumChannels.getIntValue ();

                if (newNumChannels.containsOnly ("0123456789") && configConstraints.isValidValue (Constr::numTxChannels, newNumChannelsIntValue))
                {
                    deviceValueTree.setProperty (numTxChannels, newNumChannelsIntValue, nullptr);
                    if (rootItem != nullptr)
                        rootItem->refreshSubItems ();
                }
                else
                    numTxChannelsLabel.setText (deviceValueTree.getProperty (numTxChannels), juce::NotificationType::dontSendNotification);
            };
        }

        if (configConstraints.hasFixedValue (Constr::sampleRate))
        {
            const double desiredSampleRate = configConstraints.getMinDouble (Constr::sampleRate);
            sampleRateLabel.setText (juce::String (desiredSampleRate), juce::NotificationType::dontSendNotification);
            deviceValueTree.setProperty (UHDEngine::propertySampleRate, desiredSampleRate, nullptr);
        }
        else
        {
            const double initialSampleRate = 10e6;
            sampleRateLabel.setText (juce::String (configConstraints.clipToValidValue (Constr::sampleRate, initialSampleRate)) + "Hz", juce::NotificationType::dontSendNotification);
            sampleRateLabel.setEditable (true);
            sampleRateLabel.onTextChange = [this] ()
            {
                auto newSampleRate = sampleRateLabel.getText ();
                double newSampleRateDouble = newSampleRate.getDoubleValue ();

                if (newSampleRate.containsOnly (".0123456789") && configConstraints.isValidValue (Constr::sampleRate, newSampleRateDouble))
                {
                    sampleRateLabel.setText (juce::String (newSampleRate) + "Hz", juce::NotificationType::dontSendNotification);
                    deviceValueTree.setProperty (UHDEngine::propertySampleRate, newSampleRate, nullptr);
                    if (rootItem != nullptr)
                        rootItem->refreshSubItems ();
                }
                else
                    sampleRateLabel.setText (deviceValueTree.getProperty (UHDEngine::propertySampleRate), juce::NotificationType::dontSendNotification);
            };
            deviceValueTree.setProperty (UHDEngine::propertySampleRate, initialSampleRate, nullptr);
        }

        syncSetupBox.addItem ("Internal (Single Device only)", 1);
        syncSetupBox.addItem ("External Time & PPS", 2);
        syncSetupBox.addItem ("MIMO (2 Devices only)", 3);
        syncSetupBox.setSelectedId (2, juce::NotificationType::dontSendNotification);
        deviceValueTree.setProperty (UHDEngine::propertySyncSetup, UHDEngine::SynchronizationSetup::externalSyncAndClock, nullptr);

        syncSetupBox.onChange = [this] ()
        {
            deviceValueTree.setProperty (UHDEngine::propertySyncSetup, syncSetupBox.getSelectedId () - 1, nullptr);
        };


        applyChangesButton.setButtonText ("Apply Changes");
        applyChangesButton.onClick = [this] ()
        {
            auto res = applyCurrentSettings ();
            if (res.failed ())
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "Could not apply settings", res.getErrorMessage ());
            else
            {
                juce::String readyMessage = "The USRP Engine is ready for streaming.";

                auto syncSetup = varSyncSetupConverter::fromVar (deviceValueTree.getProperty (UHDEngine::propertySyncSetup));

                if (syncSetup == UHDEngine::SynchronizationSetup::externalSyncAndClock)
                    readyMessage += "\nYou have set the synchronization setup to external time and clock.\nMake sure your time and clock source are running, otherwise streaming might fail.";

                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::InfoIcon, "Successfully applied settings", readyMessage);
            }

        };

        treeView.setDefaultOpenness (true);
        treeView.setMultiSelectEnabled (true);
        rootItem.reset (new ValueTreeItem (deviceValueTree, undoManager));
        treeView.setRootItem (rootItem.get ());

        setSize (500, 500);
    }

    void UHDEngineConfigurationComponent::constrainCenterFrequencyInTree (juce::ValueTree& tree)
    {
        auto freqRange = tree.getChildWithName (UHDEngine::propertyFreqRange);
        if (freqRange.isValid ())
        {
            const double unitScaling = freqRange.getProperty (UHDEngine::propertyUnitScaling);
            juce::Range<double> fRange (static_cast<double> (freqRange.getProperty (UHDEngine::propertyMin)) * unitScaling,
                    static_cast<double> (freqRange.getProperty (UHDEngine::propertyMax)) * unitScaling);

            const bool isRx = tree.getParent ().hasType (UHDEngine::propertyRxFrontend);

            if (isRx)
            {
                fRange = configConstraints.clipToValidRange (SDRIOEngineConfigurationInterface::ConfigurationConstraints::rxCenterFreq, fRange);
                freqRange.setProperty (UHDEngine::propertyMin, fRange.getStart () / unitScaling, nullptr);
                freqRange.setProperty (UHDEngine::propertyMax, fRange.getEnd () / unitScaling, nullptr);
            }
            else
            {
                fRange = configConstraints.clipToValidRange (SDRIOEngineConfigurationInterface::ConfigurationConstraints::txCenterFreq, fRange);
                freqRange.setProperty (UHDEngine::propertyMin, fRange.getStart () / unitScaling, nullptr);
                freqRange.setProperty (UHDEngine::propertyMax, fRange.getEnd () / unitScaling, nullptr);
            }
        }
        else
        {
            for (auto child : tree)
                constrainCenterFrequencyInTree (child);
        }
    }

    void UHDEngineConfigurationComponent::paint (juce::Graphics& g)
    {
        g.fillAll (getLookAndFeel ().findColour (juce::ResizableWindow::backgroundColourId));

        auto header = getLocalBounds ().removeFromTop (30);

        g.setColour (juce::Colours::darkgrey);
        g.fillRect (header);

        header.removeFromLeft (5);

        g.setColour (juce::Colours::white);
        g.drawText ("Number of channels: Rx", header.removeFromLeft (140), juce::Justification::centredLeft, false);
        header.removeFromLeft (30);
        g.drawText (" Tx", header.removeFromLeft (20), juce::Justification::centredLeft, false);
        header.removeFromLeft (30);
        g.drawText (" Sample Rate:", header.removeFromLeft (75), juce::Justification::centredLeft, false);
        header.removeFromLeft (60);
        g.drawText (" Sync Setup:", header.removeFromLeft (70), juce::Justification::centredLeft, false);
    }

    void UHDEngineConfigurationComponent::resized ()
    {
        auto bounds = getLocalBounds ();
        auto header = bounds.removeFromTop (30);
        header.removeFromLeft (143);
        header.removeFromRight (2);

        applyChangesButton.setBounds (header.removeFromRight (100).reduced (3));
        numRxChannelsLabel.setBounds (header.removeFromLeft (30));
        header.removeFromLeft (20);
        numTxChannelsLabel.setBounds (header.removeFromLeft (30));
        header.removeFromLeft (75);
        sampleRateLabel.setBounds (header.removeFromLeft (60));
        header.removeFromLeft (73);
        syncSetupBox.setBounds (header.removeFromLeft (200).reduced (3));

        treeView.setBounds (bounds);
    }

    UHDEngineConfigurationComponent::ChannelsAssignedMask UHDEngineConfigurationComponent::getChannelsAssignedMask (juce::ValueTree& treeRoot, const juce::Identifier& channelMaskType)
    {
        return static_cast<unsigned long long> (static_cast<juce::int64> (treeRoot.getProperty (channelMaskType)));
    }

    void UHDEngineConfigurationComponent::setChannelsAssignedMask (UHDEngineConfigurationComponent::ChannelsAssignedMask mask, juce::ValueTree& treeRoot, const juce::Identifier& channelMaskType)
    {
        treeRoot.setProperty (channelMaskType, static_cast<juce::int64> (mask.to_ullong ()), nullptr);
    }

    juce::Result UHDEngineConfigurationComponent::applyCurrentSettings ()
    {
        const auto activeRxChannelMask = getChannelsAssignedMask (deviceValueTree, rxChannelsAssigned);
        const auto activeTxChannelMask = getChannelsAssignedMask (deviceValueTree, txChannelsAssigned);
        int numRxChannelsInt = deviceValueTree.getProperty (numRxChannels);
        int numTxChannelsInt = deviceValueTree.getProperty (numTxChannels);

        if (activeRxChannelMask.count () != numRxChannelsInt)
            return juce::Result::fail ("Not all Rx channels are assigned to a frontend");

        if (activeTxChannelMask.count () != numTxChannelsInt)
            return juce::Result::fail ("Not all Tx channels are assigned to a frontend");

        juce::ValueTree activeSetup (UHDEngine::propertyUSRPDeviceConfig);

        activeSetup.setProperty (UHDEngine::propertySyncSetup, deviceValueTree.getProperty (UHDEngine::propertySyncSetup), nullptr);
        activeSetup.setProperty (UHDEngine::propertySampleRate, deviceValueTree.getProperty (UHDEngine::propertySampleRate), nullptr);

        juce::ValueTree mboardsInSetup (UHDEngine::propertyMBoards);
        activeSetup.addChild (mboardsInSetup, -1, nullptr);

        int mboardIdx = 0;

        if (numRxChannelsInt > 0)
        {
            juce::ValueTree rxChannelSetup ("Rx_Channel_Setup");
            rxChannelSetup.setProperty (UHDEngine::ChannelMapping::propertyNumChannels, deviceValueTree.getProperty (numRxChannels), nullptr);

            activeSetup.addChild (rxChannelSetup, -1, nullptr);

            createChannelSetupTree (rxChannelSetup, UHDEngine::propertyRxDboard, UHDEngine::propertyRxFrontend, mboardsInSetup, mboardIdx);
        }

        if (numTxChannelsInt > 0)
        {
            juce::ValueTree txChannelSetup ("Tx_Channel_Setup");
            txChannelSetup.setProperty (UHDEngine::ChannelMapping::propertyNumChannels, deviceValueTree.getProperty (numTxChannels), nullptr);

            activeSetup.addChild (txChannelSetup, -1, nullptr);

            createChannelSetupTree (txChannelSetup, UHDEngine::propertyTxDboard, UHDEngine::propertyTxFrontend, mboardsInSetup, mboardIdx);
        }

        return configInterface.setConfig (activeSetup);
    }

    void UHDEngineConfigurationComponent::createChannelSetupTree (juce::ValueTree& channelSetup, juce::Identifier propertyDboard, juce::Identifier propertyFrontend, juce::ValueTree& mboardsInSetup, int& mboardIdx)
    {
        for (const auto& mboard : deviceValueTree)
        {
            const auto dboards = mboard.getChildWithName (propertyDboard);
            for (const auto& dboard : dboards)
            {
                const auto frontends = dboard.getChildWithName (propertyFrontend);
                for (const auto& frontend : frontends)
                {
                    int channelAssigned = static_cast<int> (frontend.getProperty (currentComboBoxID)) - 2;
                    if (channelAssigned >= 0)
                    {
                        juce::ValueTree channelTree ("Channel_" + juce::String (channelAssigned));

                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyDboardSlot, dboard.getType ().toString (), nullptr);
                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyFrontendOnDboard, frontend.getType ().toString ().trimCharactersAtStart ("_"), nullptr);
                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyAntennaPort, frontend.getChildWithName (UHDEngine::propertyAntennas).getProperty (UHDEngine::propertyCurrentValue), nullptr);
                        // todo: work out a way to cope with various gain element names
                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyAnalogGain, 0, nullptr);

                        auto freqRange = frontend.getChildWithName (UHDEngine::propertyFreqRange);
                        auto bandwitdhRange = frontend.getChildWithName (UHDEngine::propertyBandwidthRange);

                        const double freqScaling = freqRange.getProperty (UHDEngine::propertyUnitScaling);
                        const double bandwidthScaling = bandwitdhRange.getProperty (UHDEngine::propertyUnitScaling);
                        const double currentFreq = freqRange.getProperty (UHDEngine::propertyCurrentValue);
                        const double currentBandwidth = bandwitdhRange.getProperty (UHDEngine::propertyCurrentValue);

                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyCenterFrequency, currentFreq * freqScaling, nullptr);
                        channelTree.setProperty (UHDEngine::ChannelMapping::propertyAnalogBandwitdh, currentBandwidth * bandwidthScaling, nullptr);

                        channelSetup.addChild (channelTree, -1, nullptr);

                        auto mb = mboardsInSetup.getChildWithName (mboard.getType ());
                        if (mb.isValid ())
                        {
                            channelTree.setProperty (UHDEngine::ChannelMapping::propertyMboardIdx, mb.getProperty (UHDEngine::ChannelMapping::propertyMboardIdx), nullptr);
                        }
                        else
                        {
                            juce::ValueTree mboardToAdd (mboard.getType ());
                            mboardToAdd.setProperty (UHDEngine::propertyMBoard, mboard.getProperty (UHDEngine::propertyMBoard), nullptr);
                            mboardToAdd.setProperty (UHDEngine::propertyIPAddress, mboard.getProperty (UHDEngine::propertyIPAddress, "0.0.0.0"), nullptr);
                            mboardToAdd.setProperty (UHDEngine::ChannelMapping::propertyMboardIdx, mboardIdx, nullptr);
                            channelTree.setProperty (UHDEngine::ChannelMapping::propertyMboardIdx, mboardIdx, nullptr);
                            mboardsInSetup.addChild (mboardToAdd, -1, nullptr);

                            ++mboardIdx;
                        }
                    }
                }
            }
        }
    }

    UHDEngineConfigurationComponent::RangeValueComponent::RangeValueComponent (juce::ValueTree& treeItemToReferTo)
            : treeItem (treeItemToReferTo),
              allowedValueRange (treeItem.getProperty (UHDEngine::propertyMin), treeItem.getProperty (UHDEngine::propertyMax)),
              stepWidth (treeItem.getProperty (UHDEngine::propertyStepWidth)),
              scalingFactor (treeItem.getProperty (UHDEngine::propertyUnitScaling)),
              unit (treeItem.getProperty (UHDEngine::propertyUnit).toString ())
    {
        valueDescription.setText (treeItem.getType ().toString ().replaceCharacter ('_', ' ') + " in " + unit +
                        " (Min: " + juce::String (allowedValueRange.getStart ()) + unit +
                        ", Max: " + juce::String (allowedValueRange.getEnd ()) + unit + ")",
                juce::NotificationType::dontSendNotification);

        double currentValue = treeItem.getProperty (UHDEngine::propertyCurrentValue);

        if (std::isnan (currentValue))
            previousValueEditorText = "Not specified";
        else
            previousValueEditorText = juce::String (currentValue) + unit;

        valueEditor.setText (previousValueEditorText, false);
        valueEditor.onReturnKey = [this] ()
        {
            std::stringstream valueEnteredAsString;
            double valueEnteredAsDouble;

            valueEnteredAsString << valueEditor.getText ();

            if (valueEnteredAsString >> valueEnteredAsDouble)
            {
                valueEnteredAsDouble = allowedValueRange.clipValue (valueEnteredAsDouble);
                treeItem.setProperty (UHDEngine::propertyCurrentValue, valueEnteredAsDouble, nullptr);
                previousValueEditorText = juce::String (valueEnteredAsDouble) + unit;
            }

            valueEditor.setText (previousValueEditorText, false);
        };

        addAndMakeVisible (valueDescription);
        addAndMakeVisible (valueEditor);
    }

    void UHDEngineConfigurationComponent::RangeValueComponent::resized ()
    {
        auto bounds = getLocalBounds ();
        bounds.removeFromTop (3);
        bounds.removeFromBottom (3);
        valueDescription.setBounds (bounds.removeFromLeft (250));
        valueEditor.setBounds (bounds.removeFromLeft (100));
    }

    UHDEngineConfigurationComponent::SelectionValueComponent::SelectionValueComponent (juce::ValueTree& treeItemToReferTo)
            : treeItem (treeItemToReferTo)
    {
        valueDescription.setText (treeItem.getType ().toString ().replaceCharacter ('_', ' ').trimCharactersAtEnd ("s"), juce::NotificationType::dontSendNotification);

        auto arrayValuesAsString = treeItem.getProperty (UHDEngine::propertyArray).toString ();
        auto arrayValues = juce::StringArray::fromTokens (arrayValuesAsString, ",", "\"");
        arrayValues.trim ();
        valueSelector.addItemList (arrayValues, 1);

        juce::String currentValue = treeItem.getProperty (UHDEngine::propertyCurrentValue);

        if (int id = arrayValues.indexOf (currentValue) != -1)
            valueSelector.setSelectedItemIndex (id, juce::NotificationType::dontSendNotification);

        valueSelector.onChange = [this] ()
        {
            auto currentSelection = valueSelector.getText ();
            treeItem.setProperty (UHDEngine::propertyCurrentValue, currentSelection, nullptr);
        };

        addAndMakeVisible (valueDescription);
        addAndMakeVisible (valueSelector);
    }

    void UHDEngineConfigurationComponent::SelectionValueComponent::resized ()
    {
        auto bounds = getLocalBounds ();
        bounds.removeFromTop (3);
        bounds.removeFromBottom (3);
        valueDescription.setBounds (bounds.removeFromLeft (100));
        valueSelector.setBounds (bounds.removeFromLeft (100));
    }

    UHDEngineConfigurationComponent::PropertiesComponent::PropertiesComponent (juce::ValueTree& treeItemToReferTo)
            : treeItem (treeItemToReferTo)
    {
    }

    void UHDEngineConfigurationComponent::PropertiesComponent::paint (juce::Graphics& g)
    {
        if (treeItem.getParent ().hasType (UHDEngine::propertyUSRPDevice))
            g.setColour (getLookAndFeel ().findColour (juce::Slider::ColourIds::thumbColourId));
        else
            g.setColour (juce::Colours::white);

        g.setFont (15.0f);

        g.drawText (treeItem.getType ().toString ().replaceCharacter ('_', ' '),
                4, 0, getWidth () - 4, UHDEngineConfigurationComponent::ValueTreeItem::heightPerProperty,
                juce::Justification::centredLeft, true);

        g.setColour (juce::Colours::white);

        int y = UHDEngineConfigurationComponent::ValueTreeItem::heightPerProperty;

        for (int i = 0; i < treeItem.getNumProperties (); ++i)
        {
            auto propertyName = treeItem.getPropertyName (i);
            if ((propertyName == currentComboBoxID) ||
                    (propertyName == numRxChannels) ||
                    (propertyName == numTxChannels) ||
                    (propertyName == rxChannelsAssigned) ||
                    (propertyName == txChannelsAssigned))
                continue;

            auto propertyValue = treeItem.getProperty (propertyName);

            g.drawText (propertyName.toString ().replaceCharacter ('_', ' ') + " : " + propertyValue.toString (),
                    8, y, getWidth () - 8, UHDEngineConfigurationComponent::ValueTreeItem::heightPerProperty,
                    juce::Justification::centredLeft, true);
            y += UHDEngineConfigurationComponent::ValueTreeItem::heightPerProperty;
        }
    }

    UHDEngineConfigurationComponent::FrontendPropertiesComponent::FrontendPropertiesComponent (juce::ValueTree& treeItemToReferTo)
            : PropertiesComponent (treeItemToReferTo)
    {
        if (treeItemToReferTo.getParent ().hasType (UHDEngine::propertyRxFrontend))
        {
            propertyNumChannels = numRxChannels;
            propertyChannelsAssigned = rxChannelsAssigned;
        }
        else
        {
            propertyNumChannels = numTxChannels;
            propertyChannelsAssigned = txChannelsAssigned;
        }

        addAndMakeVisible (channelSelectionBox);
        addAndMakeVisible (channelSelectionDescriptionLabel);

        channelSelectionDescriptionLabel.attachToComponent (&channelSelectionBox, true);
        channelSelectionDescriptionLabel.setText ("Assign to channel", juce::NotificationType::dontSendNotification);

        if (!treeItemToReferTo.hasProperty (currentComboBoxID))
            treeItemToReferTo.setProperty (currentComboBoxID, 1 /* -> Not assigned */, nullptr);

        treeRoot = treeItemToReferTo.getRoot ();
        treeRoot.addListener (this);

        updateChannelSelectionBoxItems ();

        channelSelectionBox.onChange = [this] ()
        {
            auto channelsAssignedMask = getChannelsAssignedMask (treeRoot, propertyChannelsAssigned);
            int selectedID = channelSelectionBox.getSelectedId ();
            int channelSelectedLast = static_cast<int> (treeItem.getProperty (currentComboBoxID)) - 2;
            int newChannelAssigned = selectedID - 2;

            if (channelSelectedLast != -1)
                channelsAssignedMask.set (static_cast<size_t> (channelSelectedLast), false);
            if (newChannelAssigned != -1)
                channelsAssignedMask.set (static_cast<size_t> (newChannelAssigned), true);

            setChannelsAssignedMask (channelsAssignedMask, treeRoot, propertyChannelsAssigned);
            treeItem.setProperty (currentComboBoxID, selectedID, nullptr);
        };
    }

    void UHDEngineConfigurationComponent::FrontendPropertiesComponent::resized ()
    {
        auto bounds = getLocalBounds ();
        bounds.removeFromLeft (200);
        channelSelectionBox.setBounds (bounds.removeFromTop (20).removeFromLeft (100));
    }

    void UHDEngineConfigurationComponent::FrontendPropertiesComponent::updateChannelSelectionBoxItems ()
    {
        const int currentNumChannels = treeRoot.getProperty (propertyNumChannels);
        auto channelsAssignedMask = getChannelsAssignedMask (treeRoot, propertyChannelsAssigned);

        if ((channelSelectionBox.getNumItems () - 1) != currentNumChannels)
        {
            juce::StringArray channelList;
            channelList.ensureStorageAllocated (currentNumChannels + 1);
            channelList.add ("Not assigned");
            for (int c = 0; c < currentNumChannels; ++c)
                channelList.add (juce::String (c));

            channelSelectionBox.clear (juce::NotificationType::dontSendNotification);
            channelSelectionBox.addItemList (channelList, 1);

            int idToSelect = treeItem.getProperty (currentComboBoxID);

            if (idToSelect > channelSelectionBox.getNumItems ())
            {
                channelSelectionBox.setSelectedId (1, juce::NotificationType::dontSendNotification);
                treeItem.setProperty (currentComboBoxID, 1, nullptr);
                channelsAssignedMask.set (static_cast<size_t> (idToSelect - 2), false);
                setChannelsAssignedMask (channelsAssignedMask, treeRoot, propertyChannelsAssigned);
            }
            else
                channelSelectionBox.setSelectedId (idToSelect, juce::NotificationType::dontSendNotification);
        }

        for (int c = 0; c < currentNumChannels; ++c)
            channelSelectionBox.setItemEnabled (c + 2, !channelsAssignedMask[c]);

    }

    void UHDEngineConfigurationComponent::FrontendPropertiesComponent::valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
    {
        if (treeWhosePropertyHasChanged.hasType (UHDEngine::propertyUSRPDevice))
        {
            if ((property == propertyNumChannels) || (property == propertyChannelsAssigned))
                updateChannelSelectionBoxItems ();
        }
    }

    UHDEngineConfigurationComponent::ValueTreeItem::ValueTreeItem (const juce::ValueTree& v, juce::UndoManager& um)
            : tree (v), undoManager (um)
    {
        tree.addListener (this);
    }

    juce::String UHDEngineConfigurationComponent::ValueTreeItem::getUniqueName () const
    {
        return tree.getType ().toString ();
    }

    bool UHDEngineConfigurationComponent::ValueTreeItem::mightContainSubItems ()
    {
        return tree.getNumChildren () > 0;
    }

    int UHDEngineConfigurationComponent::ValueTreeItem::getItemHeight () const
    {
        if (tree.hasType (UHDEngine::propertyUSRPDevice) || tree.hasType (UHDEngine::propertyTimeSources) || tree.hasType (UHDEngine::propertyClockSources))
            return 0;

        if (tree.hasProperty (UHDEngine::propertyArray) || tree.hasProperty (UHDEngine::propertyMin))
            return heightPerProperty + 6;

        return heightPerProperty * (tree.getNumProperties () + 1);
    }

    juce::Component* UHDEngineConfigurationComponent::ValueTreeItem::createItemComponent ()
    {
        if (tree.hasProperty (UHDEngine::propertyCurrentValue))
        {
            if (tree.hasProperty (UHDEngine::propertyArray))
            {
                if (tree.hasType (UHDEngine::propertyTimeSources) || tree.hasType (UHDEngine::propertyClockSources))
                    return nullptr;

                return new SelectionValueComponent (tree);
            }

            if (tree.hasProperty (UHDEngine::propertyMin))
                return new RangeValueComponent (tree);
        }

        auto parentTree = tree.getParent ();

        if (parentTree.isValid () && (parentTree.hasType (UHDEngine::propertyRxFrontend) || parentTree.hasType (UHDEngine::propertyTxFrontend)))
            return new FrontendPropertiesComponent (tree);

        return new PropertiesComponent (tree);
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::itemOpennessChanged (bool isNowOpen)
    {
        if (isNowOpen && getNumSubItems () == 0)
            refreshSubItems ();
        else
            clearSubItems ();
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::refreshSubItems ()
    {
        clearSubItems ();

        for (const auto& child : tree)
        {
            // todo: add some adjustable blacklisting
            if (child.hasType (UHDEngine::propertySensors) ||
                    child.hasType (UHDEngine::propertyRxCodec) ||
                    child.hasType (UHDEngine::propertyTxCodec) ||
                    child.hasType (UHDEngine::propertyRxDSP) ||
                    child.hasType (UHDEngine::propertyTxDSP))
                continue;

            addSubItem (new ValueTreeItem (child, undoManager));
        }
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&)
    {
        repaintItem ();
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&)
    {
        treeChildrenChanged (parentTree);
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int)
    {
        treeChildrenChanged (parentTree);
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int)
    {
        treeChildrenChanged (parentTree);
    }

    void UHDEngineConfigurationComponent::ValueTreeItem::treeChildrenChanged (const juce::ValueTree& parentTree)
    {
        if (parentTree == tree)
        {
            refreshSubItems ();
            treeHasChanged ();
            setOpen (true);
        }
    }
}