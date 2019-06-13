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
#include "../HardwareDevices/EttusEngine/UHDEngine.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace ntlab
{
    class UHDEngineConfigurationComponent : public juce::Component
    {
    public:
        static const juce::Identifier numRxChannels;
        static const juce::Identifier numTxChannels;
        static const juce::Identifier rxChannelsAssigned;
        static const juce::Identifier txChannelsAssigned;
        static const juce::Identifier currentComboBoxID;

        UHDEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints);

        void paint (juce::Graphics& g) override;

        void resized() override;

        // we use a std::bitset to manage the used channels flags and its to_ulong member function to store it as
        // int64 into a var. Therefore the maximum number of channels to be set through this component is limited.
        // However it seems unrealistic that any sdr setup might contain more channels
        static const size_t maxNumChannels = 8 * std::min (sizeof (unsigned long long), sizeof (juce::int64));
        using ChannelsAssignedMask = std::bitset<maxNumChannels>;
        static ChannelsAssignedMask getChannelsAssignedMask (juce::ValueTree& treeRoot, const juce::Identifier& channelMaskType);
        static void setChannelsAssignedMask (ChannelsAssignedMask mask, juce::ValueTree& treeRoot, const juce::Identifier& channelMaskType);

    private:
        // Used to set numerical values
        class RangeValueComponent : public juce::Component
        {
        public:
            RangeValueComponent (juce::ValueTree& treeItemToReferTo);
            void resized() override;
        private:
            juce::ValueTree treeItem;
            juce::Range<double> allowedValueRange;
            double stepWidth, scalingFactor;
            juce::Label valueDescription;
            juce::TextEditor valueEditor;
            juce::String unit, previousValueEditorText;
        };

        // Used to set a certain from a choice of possible values
        class SelectionValueComponent : public juce::Component
        {
        public:
            SelectionValueComponent (juce::ValueTree& treeItemToReferTo);
            void resized() override;
        private:
            juce::ValueTree treeItem;
            juce::Label valueDescription;
            juce::ComboBox valueSelector;
        };

        // Used to display non-settable properties of a tree item
        class PropertiesComponent : public juce::Component
        {
        public:
            PropertiesComponent (juce::ValueTree& treeItemToReferTo);
            void paint (juce::Graphics& g) override;

        protected:
            juce::ValueTree treeItem;
        };

        // In case of the frontend property this component adds a combo box to select the channel this frontend will
        // be assigned to
        class FrontendPropertiesComponent : public PropertiesComponent, private juce::ValueTree::Listener
        {
        public:
            FrontendPropertiesComponent (juce::ValueTree& treeItemToReferTo);
            void resized() override;

        private:
            juce::Identifier propertyNumChannels;
            juce::Identifier propertyChannelsAssigned;
            juce::ValueTree treeRoot;
            juce::ComboBox channelSelectionBox;
            juce::Label channelSelectionDescriptionLabel;

            void updateChannelSelectionBoxItems();

            void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
        };

        class ValueTreeItem  : public juce::TreeViewItem, private juce::ValueTree::Listener
        {
        public:
            ValueTreeItem (const juce::ValueTree& v, juce::UndoManager& um);

            juce::String getUniqueName() const override;

            bool mightContainSubItems() override;

            int getItemHeight() const override;

            juce::Component* createItemComponent() override;

            void itemOpennessChanged (bool isNowOpen) override;

            void refreshSubItems();

            static const int heightPerProperty = 20;

        private:

            juce::ValueTree tree;
            juce::UndoManager& undoManager;

            void valueTreePropertyChanged (juce::ValueTree&, const juce::Identifier&) override;
            void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree&) override;
            void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree&, int) override;
            void valueTreeChildOrderChanged (juce::ValueTree& parentTree, int, int) override;

            void treeChildrenChanged (const juce::ValueTree& parentTree);

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeItem)
        };

        SDRIOEngineConfigurationInterface& configInterface;
        SDRIOEngineConfigurationInterface::ConfigurationConstraints configConstraints;

        juce::Label numRxChannelsLabel;
        juce::Label numTxChannelsLabel;
        juce::Label sampleRateLabel;
        juce::ComboBox syncSetupBox;
        juce::TextButton applyChangesButton;

        juce::ValueTree deviceValueTree;
        juce::TreeView treeView;

        std::unique_ptr<ValueTreeItem> rootItem;
        juce::UndoManager undoManager;

        juce::Result applyCurrentSettings();
        void createChannelSetupTree (juce::ValueTree& channelSetup, juce::Identifier propertyDboard, juce::Identifier propertyFrontend, juce::ValueTree& mboardsInSetup, int& mboardIdx);
        void constrainCenterFrequencyInTree (juce::ValueTree& tree);
    };
}

