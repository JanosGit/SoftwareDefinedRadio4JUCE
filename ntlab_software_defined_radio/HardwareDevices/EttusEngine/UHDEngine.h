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

#include "../SDRIOEngine.h"
#include "UHDReplacement.h"

namespace ntlab
{
    class UHDEngine : public SDRIOHardwareEngine, private juce::Thread
    {
        friend class UHDEngineManager;
        friend class ChannelMapping;
    public:

        /**
         * A UHDEngine can manage multiple hardware devices. Therefore each in- and output channel needs to be mapped
         * to a physical port on the device. Create an array of ChannelSetups to pass it to setupRxChannels or
         * setupTxChannels to describe this mapping on a per-channel basis.
         */
        struct ChannelSetup
        {
            /**
             * Most Ettus devices contains one Motherboard only, that can carry multiple daughterboards. In this case,
             * the index maps to the device. However, check the specs of your hardware unit if that applies for your
             * device too.
             */
            int mboardIdx;
            /**
             * A motherboard often has multiple daugterboards (which contain all the analog electronic and therefore
             * determine the frequency range of the hardware). They are often named "A", "B", ...
             */
            juce::String daughterboardSlot;
            /**
             * A daugtherboard can have multiple frontends. They are often named like "0", "1", ...
             */
            juce::String frontendOnDaughterboard;
            /**
             * Some frontends can use multiple Antenna Ports, they are often named like "TX/RX", "RX2"...
             * Chosing the right antenna port can impact half/full duplex operation
             */
            juce::String antennaPort;
        };

        /**
         * For a multi USRP Setup with multiple hardware units, the user has to make sure all devices run in sync.
         * A usual way of doing this is connecting them to a central clock and time source unit that generates the ref
         * clock and the pps signal.
         */
        enum SynchronizationSetup
        {
            /** If only one device is running, it can generate its time- and clock base internally */
            singleDeviceStandalone,

            /** The usual choice for a multi-USRP setup. One central clock and time source which synchronizes all USRPs */
            externalSyncAndClock,

            /**
             * Some devices have a MIMO cable port that allows the second device to be a slave to the first one that
             * uses its internal clock
             */
            twoDevicesMIMOCableMasterSlave
        };

        /**
         * If an UHDEngine instance was created through SDRIOEngineManager::createEngine you might want to cast the
         * std::unique_ptr returned to a raw pointer to a UHDEngine instace. Therefore this is a simple wrapper
         * around a dynamic_cast for better code readability. It returns a raw pointer pointing to the instance owned
         * by the unique_ptr passed in, so make sure that the unique_ptr has a lifetime longer than the pointer returned.
         * It will return a nullprt in case the baseEnginePtr passed is not an UHDEngine instance.
         */
        static UHDEngine* castToUHDEnginePtr (std::unique_ptr<SDRIOEngine>& baseEnginePtr);

        /**
         * If an UHDEngine instance was created through SDRIOEngineManager::createEngine you might want to cast the
         * std::unique_ptr returned to a reference to a UHDEngine instace. Therefore this is a simple wrapper
         * around a dynamic_cast for better code readability. It returns a reference pointing to the instance
         * owned by the unique_ptr passed in, so make sure that the unique_ptr has a lifetime longer than the reference
         * returned. It will throw a std::bad_cast in case the baseEnginePtr passed is not an UHDEngine instance, so
         * better only use it if you are absolutely sure that the baseEnginePtr holds a UHDEngine instance.
         */
        static UHDEngine& castToUHDEngineRef (std::unique_ptr<SDRIOEngine>& baseEnginePtr);

        ~UHDEngine();

        /**
         * Create the USRP instance used under the hood of this engine to communicate with the Hardware. No streaming
         * will be possible before having successfully called this command. For a documentation of the possible
         * arguments refer to the Ettus documentation, you'll most likely only set IP Adresses as args for a simple
         * setup. Don't forget to either set up rx or tx channels afterwards.
         */
        juce::Result makeUSRP (juce::StringPairArray& args, SynchronizationSetup synchronizationSetup);

        /**
         * The most common use-case is to create a multi URSP instance through IP addresses. This is a convenient
         * wrapper for this use-case.
         */
        juce::Result makeUSRP (juce::Array<juce::IPAddress> ipAddresses, SynchronizationSetup synchronizationSetup);

        /**
         * To receive samples, any UHD setup needs some initial channel mapping. See the comment above ChannelSetup
         * for a more detailed information.
         * @see ChannelSetup
         */
        juce::Result setupRxChannels (juce::Array<ChannelSetup>& channelSetup);

        /**
         * To receive samples, any UHD setup needs some initial channel mapping. See the comment above ChannelSetup
         * for a more detailed information.
         * @see ChannelSetup
         */
        juce::Result setupTxChannels (juce::Array<ChannelSetup>& channelSetup);

        bool setSampleRate (double newSampleRate) override;

        double getSampleRate() override;

        const juce::var& getDeviceTree() override;

        juce::var getActiveConfig() override;

        bool setConfig (juce::var& configToSet) override;

        bool setDesiredBlockSize (int desiredBlockSize) override;

        bool isReadyToStream() override;

        bool startStreaming (SDRIODeviceCallback* callback) override;

        void stopStreaming() override;

        bool isStreaming() override;

        bool enableRxTx (bool enableRx, bool enableTx) override;

        bool setRxCenterFrequency (double newCenterFrequency, int channel) override;

        double getRxCenterFrequency (int channel) override;

        bool setRxBandwidth (double newBandwidth, int channel) override;

        double getRxBandwidth (int channel) override;

        //double getRxLOFrequency (int channel = allChannels) override;

        bool setRxGain (double newGain, GainElement gainElement, int channel) override;

        double getRxGain (int channel, GainElement gainElement) override;

        bool setTxCenterFrequency (double newCenterFrequency, int channel) override;

        double getTxCenterFrequency (int channel) override;

        bool setTxBandwidth (double newBandwidth, int channel) override;

        double getTxBandwidth (int channel) override;

        //double getTxLOFrequency (int channel = allChannels) override;

        bool setTxGain (double newGain, GainElement gainElement, int channel) override;

        double getTxGain (int channel, GainElement gainElement) override;

    private:
        // manages the mapping between the buffer channels exposed to the public interface and the hardware ressources
        class ChannelMapping
        {
        public:
            ChannelMapping (juce::Array<ChannelSetup>& channelSetup, UHDEngine& engine);

            int getMboardForBufferChannel (int bufferChannel);

            const juce::String& getDboardForBufferChannel (int bufferChannel);

            const juce::String& getFrontendForBufferChannel (int bufferChannel);

            const juce::StringArray& getSubdevSpecs();

            size_t* getStreamArgsChannelList();

            const int numChannels;

        private:
            juce::Array<ChannelSetup> channelSetupHardwareOrder;
            juce::Array<size_t> bufferOrderToHardwareOrder;
            juce::StringArray subdevSpecs;
            UHDEngine& uhdEngine;
        };

        UHDEngine (UHDr::Ptr uhdLibrary);

        UHDr::Ptr uhdr;

        // The device tree holds all devices available, however not all devices might be used in a multi usrp setup
        juce::var deviceTree;
        int numMboardsInDeviceTree = 0;
        juce::StringArray deviceTypesInDeviceTree;
        juce::Array<std::reference_wrapper<juce::var>> mboardsInDeviceTree;

        static const juce::Identifier propertyUSRPDevice;
        static const juce::Identifier propertyIPAddress;

        UHDr::USRP::Ptr usrp;
        int numMboardsInUSRP = 0;
        juce::Array<int> devicesInUSRP; // indexes referring to the device tree arrays above
        SynchronizationSetup synchronizationSetup;

        std::unique_ptr<ChannelMapping> rxChannelMapping;
        std::unique_ptr<ChannelMapping> txChannelMapping;

        std::unique_ptr<UHDr::USRP::RxStream> rxStream;
        std::unique_ptr<UHDr::USRP::TxStream> txStream;

        bool rxEnabled = false;
        bool txEnabled = false;

        juce::Array<UHDr::TuneResult> rxTuneResults;
        juce::Array<UHDr::TuneResult> txTuneResults;

        int desiredBlockSize = 1024;

        // default streaming args
        static const juce::String defaultCpuFormat;
        static const juce::String defaultOtwFormat;
        static const juce::String defaultArgs;

        juce::String lastError;

        SDRIODeviceCallback* activeCallback = nullptr;

        void run() override;

        const juce::var& getDeviceTreeUpdateIfNotPresent();

        juce::IPAddress getIPAddressForMboard (int mboardIdx);
    };

    class UHDEngineManager : public SDRIOEngineManager
    {
    public:
        juce::String getEngineName() override;

        juce::Result isEngineAvailable() override;

    protected:
        SDRIOEngine* createEngine() override;

    private:
        UHDr::Ptr uhdr;
    };
}