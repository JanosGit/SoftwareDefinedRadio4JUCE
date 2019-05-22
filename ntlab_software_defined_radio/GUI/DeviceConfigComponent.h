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

#if JUCE_MODULE_AVAILABLE_juce_gui_basics

#include <juce_gui_basics/juce_gui_basics.h>
#include "../HardwareDevices/SDRIOEngine.h"

namespace ntlab
{
    class DeviceConfigComponent : public juce::TreeView
    {
    public:
        DeviceConfigComponent (SDRIOEngineConfigurationInterface* configurationInterface = nullptr);

    private:

        class DeviceTreeLeaf : public juce::TreeViewItem
        {
        public:
            static DeviceTreeLeaf* createReadOnlyLeaf (juce::Identifier propertyName, juce::DynamicObject* propertyDetails, juce::var currentValue = juce::var());

            static DeviceTreeLeaf* createEditableLeaf (juce::Identifier propertyName, juce::DynamicObject* propertyDetails, juce::var currentValue = juce::var());

            bool mightContainSubItems() override {return false; }
            void paintItem(juce::Graphics& g, int width, int height) override;

        private:
            struct DeviceTreeLeafComponent : public juce::Component
            {
                DeviceTreeLeafComponent (juce::Identifier& propertyName, juce::DynamicObject* propertyDetails, bool editable, juce::var currentValue);
                void paint (juce::Graphics&) override;
                void resized() override;
                juce::DynamicObject* property;
                juce::Label propertyValueLabel, propertyMinLabel, propertyMaxLabel;
                juce::Identifier propertyName;
                juce::String propertyUnit;
                double stepWidth = -1.0;
                juce::Range<double> valueRange;
                static const int labelHeight = 15;
            };

            DeviceTreeLeaf (juce::Identifier& propertyName, juce::DynamicObject* propertyDetails, bool editable, juce::var currentValue);

        };

        static const juce::Identifier propertyMin;
        static const juce::Identifier propertyMax;
        static const juce::Identifier propertyStepWidth;
        static const juce::Identifier propertyUnit;
    };
}

#endif