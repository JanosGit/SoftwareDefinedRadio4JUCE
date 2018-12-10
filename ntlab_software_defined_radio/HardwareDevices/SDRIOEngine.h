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

#include "SDRIODeviceCallback.h"


namespace ntlab
{

    /**
     * A base class for all SDRIOEngines. Note that those functions are not intended to be called directly but should
     * be invoked through the SDRIODeviceManager
     */
    class SDRIOEngine
    {
    public:
        virtual ~SDRIOEngine() {};

        /** Returns the name for this engine */
        virtual juce::String getName() = 0;

        /**
         * As an engine might handle multiple devices with heterogenous hardware capabilities this call returns a tree
         * of all available devices, sub devices, etc. depending on the actual structure of the engine.
         */
        virtual juce::var getDeviceTree() = 0;

        /** Returns the currently active config if any exists */
        virtual juce::var getActiveConfig() = 0;

        /** Applies a complete config. If this was successful it returns true, otherwise it returns false */
        virtual bool setConfig (juce::var& configToSet) = 0;

        /** Returns true if the engine is configured and can start to stream */
        virtual bool isReadyToStream() = 0;

        /**
         * The engine should call SDRIODeviceCallback::prepareForStreaming first and then start to call
         * SDRIODeviceCallback::processRFSampleBlock on a regular basis
         */
        virtual bool startStreaming (SDRIODeviceCallback* callback) = 0;

        /** The engine should stop streaming and call SDRIODeviceCallback::streamingHasStopped afterwards */
        virtual void stopStreaming() = 0;

        /** Returns true if the engine is currenctly streaming */
        virtual bool isStreaming() = 0;
    };
}