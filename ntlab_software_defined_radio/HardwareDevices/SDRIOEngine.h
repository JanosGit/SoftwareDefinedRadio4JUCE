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
     * be invoked through the SDRIODeviceManager. Also note that you should supply a corresponding SDRIOEngineManager
     * subclass.
     *
     * @see SDRIOEngineManager
     */
    class SDRIOEngine
    {
    public:
        virtual ~SDRIOEngine() {};

        static const int allChannels = -1;

        /**
         * As an engine might handle multiple devices with heterogenous hardware capabilities this call returns a tree
         * of all available devices, sub devices, etc. depending on the actual structure of the engine.
         */
        virtual const juce::var& getDeviceTree() = 0;

        /** Returns the currently active config if any exists */
        virtual juce::var getActiveConfig() = 0;

        /** Applies a complete config. If this was successful it returns true, otherwise it returns false */
        virtual bool setConfig (juce::var& configToSet) = 0;

        /**
         * Tries to set a desired block size. If true is returned it is guaranteed that no block passed to the callback
         * is bigger than the desired block size, however smaller blocks can occur at any time.
         */
        virtual bool setDesiredBlockSize (int desiredBlockSize) = 0;

        /**
         * Tries to set the sample rate used for input and output. Multi-samplerate setups are not supported at this
         * time. Returns true if the exact sample rate could be applied, false otherwise. Check getSampleRate() in this
         * case to find out if a sample rate value near to the desired value was applied.
         */
        virtual bool setSampleRate (double newSampleRate) = 0;

        /** Returns the current sample rate used for input and output. Returns 0 if no sample rate is configured */
        virtual double getSampleRate() = 0;

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

        /**
         * Allows you to enable and disable rx and tx, also from within the SDRIODeviceCallback::processRFSampleBlock
         * callback. This allows full or half duplex applications, depending on the hardware capabilities. Returns true
         * if the setting could be applied, false otherwise. E.g. if the hardware supports only half duplex a call to
         * enableRxTxChannels(true, true) would fail while enableRxTxChannels(true, false) or
         * enableRxTxChannels(false, true) would both be successful. Disabling both, in- and output will fail in every
         * case.
         * If you call this from withing the SDRIODeviceCallback::processRFSampleBlock callback settings will be applied
         * in the next callback.
         */
        virtual bool enableRxTx (bool enableRx, bool enableTx) = 0;
    };

    /**
     * A subclass of this is needed for every SDRIOEngine subclass. An SDRIOEngineManager can check the availabylity
     * of an SDRIOEngine on a particular system at runtime without creating a whole instance of the engine by a call
     * to isEngineAvailable. The static methods of this class help to hold all available engines managers and create
     * an engine by name.
     */
    class SDRIOEngineManager
    {
    public:
        virtual ~SDRIOEngineManager () {};

        /** Returns an array of all engines that can be created */
        static juce::StringArray getAvailableEngines();

        /** Register an SDREngineManager subclass to be able to use it to create a specific engine */
        static void registerSDREngine (SDRIOEngineManager* engineManager);

        /** Register all engines that come with this module */
        static void registerDefaultEngines();

        /**
         * Clears all Engines from the SDRIOEngineManager.
         * Although this is not really needed for a working release build, calling this at shutdown of your app will
         * prevent one or more false positive juce leak detector assertions that migh be thrown because of an
         * inverted deletion order of the static array owning the engines and the static leak detector.
         */
        static void clearAllRegisteredEngines();

        /**
         * Create an instance of the engine with this name. Get a list of all available engines through a call to
         * getAvailableEngines. Returns a nullpointer in case this engine could not be created.
         */
        static std::unique_ptr<SDRIOEngine> createEngine (const juce::String& engineName);

        virtual juce::String getEngineName() = 0;

        virtual juce::Result isEngineAvailable() = 0;
    protected:
        virtual SDRIOEngine* createEngine() = 0;

    private:

        static juce::OwnedArray<SDRIOEngineManager> managers;
    };

    /**
     * A base class for all SDR engines that actually represent hardware devices and therefore are tunable
     */
    class SDRIOHardwareEngine : public SDRIOEngine
    {
    public:
        enum GainElement
        {
            unspecified,
            analogPreADC,
            analogPostDAC,
            digital
        };

        virtual ~SDRIOHardwareEngine () {};

        virtual bool setRxCenterFrequency (double newCenterFrequency, int channel = allChannels) = 0;

        /** Returns the current Rx Center Frequency or -1.0 in case of an error */
        virtual double getRxCenterFrequency (int channel) = 0;

        virtual bool setRxBandwidth (double newBandwidth, int channel = allChannels) = 0;

        virtual double getRxBandwidth (int channel) = 0;

        virtual double getRxLOFrequency (int channel = allChannels) {return 0.0;}

        virtual bool setRxGain (double newGain, GainElement gainElement = GainElement::unspecified, int channel = allChannels) = 0;

        virtual double getRxGain (int channel, GainElement gainElement = GainElement::unspecified) = 0;

        virtual bool setTxCenterFrequency (double newCenterFrequency, int channel = allChannels) = 0;

        virtual double getTxCenterFrequency (int channel) = 0;

        virtual bool setTxBandwidth (double newBandwidth, int channel = allChannels) = 0;

        virtual double getTxBandwidth (int channel) = 0;

        virtual double getTxLOFrequency (int channel = allChannels) {return 0.0;}

        virtual bool setTxGain (double newGain, GainElement gainElement = GainElement::unspecified, int channel = allChannels) = 0;

        virtual double getTxGain (int channel, GainElement gainElement = GainElement::unspecified) = 0;
    };
}