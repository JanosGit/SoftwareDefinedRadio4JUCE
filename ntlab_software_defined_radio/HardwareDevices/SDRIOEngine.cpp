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
#include "HackRFEngine/HackRfEngine.h"

namespace ntlab
{
    void SDRIOHardwareEngine::addTuneChangeListener (TuneChangeListener* newListener)
    {
        jassert (newListener != nullptr);

        tuneChangeListeners.addIfNotAlreadyThere (newListener);

        const int numRxChannels = getNumRxChannels();
        for (int rxChannel = 0; rxChannel < numRxChannels; ++rxChannel)
        {
            newListener->rxBandwidthChanged (getRxBandwidth (rxChannel), rxChannel);
            newListener->rxCenterFreqChanged (getRxCenterFrequency (rxChannel), rxChannel);
        }

        const int numTxChannels = getNumTxChannels();
        for (int txChannel = 0; txChannel < numTxChannels; ++txChannel)
        {
            newListener->txBandwidthChanged (getTxBandwidth (txChannel), txChannel);
            newListener->txCenterFreqChanged (getTxCenterFrequency (txChannel), txChannel);
        }
    }

    void SDRIOHardwareEngine::removeTuneChangeListener (TuneChangeListener* listenerToRemove)
    {
        tuneChangeListeners.removeFirstMatchingValue (listenerToRemove);
    }

    void SDRIOHardwareEngine::notifyListenersRxBandwidthChanged (double newRxBandwidth, int channel)
    {
        for (auto* listener : tuneChangeListeners)
            listener->rxBandwidthChanged (newRxBandwidth, channel);
    }

    void SDRIOHardwareEngine::notifyListenersRxCenterFreqChanged (double newRxCenterFreq, int channel)
    {
        for (auto* listener : tuneChangeListeners)
            listener->rxCenterFreqChanged (newRxCenterFreq, channel);
    }

    void SDRIOHardwareEngine::notifyListenersTxBandwidthChanged (double newTxBandwidth, int channel)
    {
        for (auto* listener : tuneChangeListeners)
            listener->txBandwidthChanged (newTxBandwidth, channel);
    }

    void SDRIOHardwareEngine::notifyListenersTxCenterFreqChanged (double newTxCenterFreq, int channel)
    {
        for (auto* listener : tuneChangeListeners)
            listener->txCenterFreqChanged (newTxCenterFreq, channel);
    }

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

#if !JUCE_IOS
        if (!currentEngines.contains ("UHD Engine"))
        {
            auto uhdEngineManager = std::make_unique<UHDEngineManager>();
            if (uhdEngineManager->isEngineAvailable())
                managers.add (uhdEngineManager.release());
        }

        if (!currentEngines.contains ("HackRF Engine"))
        {
            auto hackRFEngineManager = std::make_unique<HackRFEngineManager>();
            if (hackRFEngineManager->isEngineAvailable())
                managers.add (hackRFEngineManager.release());
        }
#endif

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

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
    std::unique_ptr<juce::Component> SDRIOEngineManager::createEngineConfigurationComponent (const juce::String& engineName, ntlab::SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints)
    {
        for (auto manager : managers)
        {
            if (manager->getEngineName() == engineName)
            {
                if (manager->isEngineAvailable())
                {
                    return manager->createEngineConfigurationComponent (configurationInterface, constraints);
                }
                else
                {
                    return nullptr;
                }
            }
        }

        return nullptr;
    }
#endif
}