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

#include "DeviceConfigComponent.h"

#if JUCE_MODULE_AVAILABLE_juce_gui_basics

namespace ntlab
{
    const juce::Identifier DeviceConfigComponent::propertyMin       ("min");
    const juce::Identifier DeviceConfigComponent::propertyMax       ("max");
    const juce::Identifier DeviceConfigComponent::propertyStepWidth ("step width");
    const juce::Identifier DeviceConfigComponent::propertyUnit      ("unit");


    DeviceConfigComponent::DeviceConfigComponent (ntlab::SDRIOEngineConfigurationInterface* configurationInterface)
    {
        auto tree = ntlab::parseUHDUSRPProbe (nullptr, nullptr);
        auto device = tree.getProperty ("USRP Device", juce::var());
        auto device0 = device.getProperty ("USRP2 / N-Series Device 0", juce::var());
        auto rxDSP = device0.getProperty ("RX DSP", juce::var());
        auto rxDSP0 = rxDSP.getProperty ("0", juce::var());
        auto freqRange = rxDSP0.getProperty ("Freq range", juce::var());



        DeviceTreeLeaf* item = DeviceTreeLeaf::createEditableLeaf ("Frequency", freqRange.getDynamicObject());
        setRootItem (item);
    }

    DeviceConfigComponent::DeviceTreeLeaf* DeviceConfigComponent::DeviceTreeLeaf::createReadOnlyLeaf (juce::Identifier propertyName, juce::DynamicObject* propertyDetails, juce::var currentValue)
    {
        return new DeviceTreeLeaf (propertyName, propertyDetails, false, currentValue);
    }

    DeviceConfigComponent::DeviceTreeLeaf* DeviceConfigComponent::DeviceTreeLeaf::createEditableLeaf (juce::Identifier propertyName, juce::DynamicObject* propertyDetails, juce::var currentValue)
    {
        return new DeviceTreeLeaf (propertyName, propertyDetails, true, currentValue);
    }

    void DeviceConfigComponent::DeviceTreeLeaf::paintItem (juce::Graphics& g, int width, int height)
    {
        g.fillAll(juce::Colours::grey);
        g.setColour(juce::Colours::black);
        g.drawText("item", 0, 0, width, height, juce::Justification::left);
    }

    DeviceConfigComponent::DeviceTreeLeaf::DeviceTreeLeafComponent::DeviceTreeLeafComponent (juce::Identifier& pn, juce::DynamicObject* propertyDetails, bool editable, juce::var currentValue)
    {
        propertyName = pn;
        auto details = propertyDetails->getProperties();

        if (details.contains (DeviceConfigComponent::propertyUnit))
            propertyUnit = details.getVarPointer (DeviceConfigComponent::propertyUnit)->toString();

        if (details.contains (DeviceConfigComponent::propertyStepWidth))
            stepWidth = details.getWithDefault (DeviceConfigComponent::propertyStepWidth, -1.0);

        if (details.contains (DeviceConfigComponent::propertyMin) && details.contains (DeviceConfigComponent::propertyMax))
        {
            valueRange.setStart (details.getWithDefault (DeviceConfigComponent::propertyMin, std::numeric_limits<double>::min()));
            valueRange.setEnd   (details.getWithDefault (DeviceConfigComponent::propertyMax, std::numeric_limits<double>::max()));
        }

        propertyValueLabel.setEditable (editable, false, true);
        propertyValueLabel.setText (currentValue.toString(), juce::NotificationType::dontSendNotification);
        if (editable)
        {
            propertyValueLabel.onTextChange = [this] ()
            {
                auto newValue = propertyValueLabel.getText (false);
                propertyValueLabel.setText (newValue + propertyUnit, juce::NotificationType::dontSendNotification);

                if (!valueRange.isEmpty())
                {
                    double doubleValue = newValue.getDoubleValue();
                    if (valueRange.contains (doubleValue))
                    {
                        // todo: Callback
                        DBG ("New valid value for " << propertyName.toString() << ": " << doubleValue);
                    }
                    else
                    {
                        // todo: Some warning
                        DBG ("Invalid value for " << propertyName.toString());
                    }
                }
            };
        }

        addAndMakeVisible (propertyValueLabel);

        if (!valueRange.isEmpty())
        {
            propertyMinLabel.setText ("Min: " + juce::String (valueRange.getStart(), 6) + propertyUnit, juce::NotificationType::dontSendNotification);
            propertyMaxLabel.setText ("Max: " + juce::String (valueRange.getEnd(),   6) + propertyUnit, juce::NotificationType::dontSendNotification);

            addAndMakeVisible (propertyMinLabel);
            addAndMakeVisible (propertyMaxLabel);
        }
    }

    void DeviceConfigComponent::DeviceTreeLeaf::DeviceTreeLeafComponent::paint (juce::Graphics& g)
    {
        g.fillAll (findColour (ColourIds::backgroundColourId, true));
        g.setFont (juce::Font (12));
        g.setColour (juce::Colours::white);
        g.drawText (propertyName.toString(), getLocalBounds(), juce::Justification::topLeft, true);
    }

    void DeviceConfigComponent::DeviceTreeLeaf::DeviceTreeLeafComponent::resized()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop (labelHeight);

        propertyValueLabel.setBounds (bounds.removeFromTop (labelHeight));

        if (!valueRange.isEmpty())
        {
            bounds.removeFromLeft (10);
            propertyMinLabel.setBounds (bounds.removeFromTop (labelHeight));
            propertyMaxLabel.setBounds (bounds.removeFromTop (labelHeight));
        }
    }

    DeviceConfigComponent::DeviceTreeLeaf::DeviceTreeLeaf (juce::Identifier& propertyName, juce::DynamicObject* propertyDetails, bool editable, juce::var currentValue)
    {
        //todo
    }
}

#endif