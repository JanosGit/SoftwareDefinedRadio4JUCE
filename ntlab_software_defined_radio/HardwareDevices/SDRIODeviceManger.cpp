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
    void SDRIODeviceManager::addDefaultEngines ()
    {
        addEngine (new MCVFileEngine);
    }

    bool SDRIODeviceManager::addEngine (ntlab::SDRIOEngine* engineToAddAndOwn)
    {
        if (engineToAddAndOwn == nullptr)
            return false;

        if (enginesAvailable.contains (engineToAddAndOwn))
            return false;

        enginesAvailable.add (engineToAddAndOwn);
        engineNamesAvailable.add (engineToAddAndOwn->getName());

        return true;
    }


    std::vector<std::reference_wrapper<SDRIOEngine>> SDRIODeviceManager::getEngines()
    {
        std::vector<std::reference_wrapper<SDRIOEngine>> engines;

        for (auto engine : enginesAvailable)
            engines.push_back (*engine);

        return engines;
    }

    juce::StringArray SDRIODeviceManager::getEngineNames () {return engineNamesAvailable; }

    bool SDRIODeviceManager::selectEngine (juce::String& engineName)
    {
        if (!engineNamesAvailable.contains (engineName))
            return false;

        int idx = engineNamesAvailable.indexOf (engineName);

        selectedEngine = enginesAvailable[idx];

        return true;
    }

    SDRIOEngine& SDRIODeviceManager::getSelectedEngine () {return *selectedEngine; }

    juce::String SDRIODeviceManager::getSelectedEngineName () {return selectedEngine->getName(); }

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