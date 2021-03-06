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

#include <juce_core/juce_core.h>
#include "SDRIOEngine.h"

namespace ntlab
{

    class SDRIODeviceManager
    {
    public:

        SDRIODeviceManager();

        ~SDRIODeviceManager();

        /** Adds all engines that come with this module */
        void addDefaultEngines();

        /** Adds a custom engine. Optionally the engine can be selected immediately after adding it */
        void addEngine (std::unique_ptr<SDRIOEngineManager>&& engineManager, bool selectEngine);

        /** Retuns an array of the name of all engines currently managed by this instance */
        juce::StringArray getEngineNames();

        /**
         * Selects an engine to be used for streaming. Returns true if an engine with this name could be selected, false
         * otherwise. Note that in order to invoke streaming you first need to apply a valid config to the engine and
         * start streaming with a call to startStreaming();
         */
        bool selectEngine (juce::StringRef engineName);

        SDRIOEngine* getSelectedEngine();

        juce::String getSelectedEngineName();

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
        std::unique_ptr<juce::Component> getConfigurationComponentForSelectedEngine (SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints = SDRIOEngineConfigurationInterface::ConfigurationConstraints::unconstrained());
#endif

        void setCallback (SDRIODeviceCallback* callback);

        /** Returns true if an engine is configured and can start to stream */
        bool isReadyToStream();

        /** Starts streaming and calls prepareForStreaming before. Returns false in case streaming couldn't be started */
        bool startStreaming();

        /** Stops streaming and calls SDRIODeviceCallback::streamingHasStopped afterwards */
        void stopStreaming();

    private:
        std::unique_ptr<SDRIOEngine> selectedEngine;
        juce::String selectedEngineName;
        SDRIODeviceCallback* callbackToUse = nullptr;

        static std::atomic<int> numManagersActive;
    };
}