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

#include "SDRIOEngine.h"

#include "MCVFileEngine/MCVFileEngine.h"
#include "EttusEngine/UHDEngine.h"

namespace ntlab
{

    juce::OwnedArray<SDRIOEngineManager> SDRIOEngineManager::managers;

    juce::StringArray SDRIOEngineManager::getAvailableEngines ()
    {
        juce::StringArray engineNames;
        for (auto manager : managers)
        {
            if (manager->isEngineAvailable())
            {
                engineNames.add (manager->getEngineName());
            }
        }

        return engineNames;
    }

    void SDRIOEngineManager::registerSDREngine (ntlab::SDRIOEngineManager* engineManager)
    {
        managers.add (engineManager);
    }

    void SDRIOEngineManager::registerDefaultEngines ()
    {
        auto currentEngines = getAvailableEngines();

        if (!currentEngines.contains ("UHD Engine"))
        {
            std::unique_ptr<UHDEngineManager> uhdEngineManager (new UHDEngineManager);
            if (uhdEngineManager->isEngineAvailable())
                managers.add (uhdEngineManager.release());
        }

        if (!currentEngines.contains ("MCV File Engine"))
        {
            std::unique_ptr<MCVFileEngineManager> mcvFileEngineManager (new MCVFileEngineManager);
            if (mcvFileEngineManager->isEngineAvailable())
                managers.add (mcvFileEngineManager.release());
        }
    }

    void SDRIOEngineManager::clearAllRegisteredEngines ()
    {
        managers.clear (true);
    }

    std::unique_ptr<SDRIOEngine> SDRIOEngineManager::createEngine (const juce::String& engineName)
    {
        for (auto manager : managers)
        {
            if (manager->getEngineName() == engineName)
            {
                if (manager->isEngineAvailable())
                {
                    return std::unique_ptr<SDRIOEngine> (manager->createEngine());
                }
                else
                {
                    return nullptr;
                }
            }
        }

        return nullptr;
    }
}