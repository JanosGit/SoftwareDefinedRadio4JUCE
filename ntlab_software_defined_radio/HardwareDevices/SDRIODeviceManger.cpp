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

#include "SDRIODeviceManger.h"
#include "MCVFileEngine/MCVFileEngine.h"

namespace ntlab
{
    std::atomic<int> SDRIODeviceManager::numManagersActive (0);

    SDRIODeviceManager::SDRIODeviceManager()
    {
        ++numManagersActive;
    }

    SDRIODeviceManager::~SDRIODeviceManager ()
    {
        --numManagersActive;
        if (numManagersActive == 0)
            SDRIOEngineManager::clearAllRegisteredEngines();
    }

    void SDRIODeviceManager::addDefaultEngines ()
    {
        SDRIOEngineManager::registerDefaultEngines();
    }

    juce::StringArray SDRIODeviceManager::getEngineNames () {return SDRIOEngineManager::getAvailableEngines(); }

    bool SDRIODeviceManager::selectEngine (juce::StringRef engineName)
    {
        auto newEngine = SDRIOEngineManager::createEngine (engineName);
        if (newEngine == nullptr)
            return false;

        selectedEngine = std::move (newEngine);
        selectedEngineName = engineName;
        return true;
    }

    SDRIOEngine* SDRIODeviceManager::getSelectedEngine () {return selectedEngine.get(); }

    juce::String SDRIODeviceManager::getSelectedEngineName () {return selectedEngineName; }

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
    std::unique_ptr<juce::Component> SDRIODeviceManager::getConfigurationComponentForSelectedEngine ()
    {
        if (selectedEngine == nullptr)
            return nullptr;

        auto configComponent = SDRIOEngineManager::createEngineConfigurationComponent (selectedEngineName, *selectedEngine.get());

        if (configComponent == nullptr)
        {
            auto warningLabel = new juce::Label ("", "Warning: No configuration component implemented for " + selectedEngineName);
            warningLabel->setColour (juce::Label::ColourIds::textColourId, juce::Colours::red);
            warningLabel->setSize (200, 50);
            configComponent.reset (warningLabel);
        }

        return configComponent;
    }
#endif

    void SDRIODeviceManager::setCallback (ntlab::SDRIODeviceCallback* callback) {callbackToUse = callback; }

    bool SDRIODeviceManager::isReadyToStream ()
    {
        if (selectedEngine == nullptr)
            return false;

        if (callbackToUse == nullptr)
            return false;

        return selectedEngine->isReadyToStream();
    }

    bool SDRIODeviceManager::startStreaming ()
    {
        if (selectedEngine == nullptr)
            return false;

        if (callbackToUse == nullptr)
            return false;

        return selectedEngine->startStreaming (callbackToUse);
    }

    void SDRIODeviceManager::stopStreaming ()
    {
        if (selectedEngine != nullptr)
            selectedEngine->stopStreaming();
    }
}