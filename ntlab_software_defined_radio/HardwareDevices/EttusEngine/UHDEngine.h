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

    class UHDEngine
#if !JUCE_IOS
            : public SDRIOHardwareEngine, private juce::Thread
    {
        friend class UHDEngineManager;
        friend class ChannelMapping;
        friend class UHDEngineConfigurationComponent;
#else
    {
#endif
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
            singleDeviceStandalone = 0,

            /** The usual choice for a multi-USRP setup. One central clock and time source which synchronizes all USRPs */
            externalSyncAndClock = 1,

            /**
             * Some devices have a MIMO cable port that allows the second device to be a slave to the first one that
             * uses its internal clock
             */
            twoDevicesMIMOCableMasterSlave = 2
        };

        static const juce::Identifier propertyUSRPDevice;
        static const juce::Identifier propertyUSRPDeviceConfig;
        static const juce::Identifier propertyMBoards;
        static const juce::Identifier propertyMBoard;
        static const juce::Identifier propertyTimeSources;
        static const juce::Identifier propertyClockSources;
        static const juce::Identifier propertySensors;
        static const juce::Identifier propertyRxDSP;
        static const juce::Identifier propertyTxDSP;
        static const juce::Identifier propertyRxDboard;
        static const juce::Identifier propertyTxDboard;
        static const juce::Identifier propertyRxFrontend;
        static const juce::Identifier propertyTxFrontend;
        static const juce::Identifier propertyRxCodec;
        static const juce::Identifier propertyTxCodec;
        static const juce::Identifier propertyIPAddress;
        static const juce::Identifier propertyID;
        static const juce::Identifier propertyName;
        static const juce::Identifier propertySerial;
        static const juce::Identifier propertyMin;
        static const juce::Identifier propertyMax;
        static const juce::Identifier propertyStepWidth;
        static const juce::Identifier propertyUnit;
        static const juce::Identifier propertyUnitScaling;
        static const juce::Identifier propertyCurrentValue;
        static const juce::Identifier propertyArray;
        static const juce::Identifier propertyFreqRange;
        static const juce::Identifier propertyBandwidthRange;
        static const juce::Identifier propertyAntennas;
        static const juce::Identifier propertySyncSetup;
        static const juce::Identifier propertySampleRate;

#if JUCE_IOS
    };
#else

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

        const int getNumRxChannels() override;

        const int getNumTxChannels() override;

        /**
         * To receive samples, any UHD setup needs some initial channel mapping. See the comment above ChannelSetup
         * for a more detailed information.
         * @see ChannelSetup
         */
        juce::Result setupRxChannels (const juce::Array<ChannelSetup>& channelSetup);

        /**
         * To receive samples, any UHD setup needs some initial channel mapping. See the comment above ChannelSetup
         * for a more detailed information.
         * @see ChannelSetup
         */
        juce::Result setupTxChannels (const juce::Array<ChannelSetup>& channelSetup);

        bool setSampleRate (double newSampleRate) override;

        double getSampleRate() override;

        juce::ValueTree getDeviceTree() override;

        juce::ValueTree getActiveConfig() override;

        juce::Result setConfig (juce::ValueTree& configToSet) override;

        bool setDesiredBlockSize (int desiredBlockSize) override;

        bool isReadyToStream() override;

        bool startStreaming (SDRIODeviceCallback* callback) override;

        void stopStreaming() override;

        bool isStreaming() override;

        bool enableRxTx (RxTxState rxTxState) override;

        bool setRxCenterFrequency (double newCenterFrequency, int channel) override;

        double getRxCenterFrequency (int channel) override;

        bool setRxBandwidth (double newBandwidth, int channel) override;

        double getRxBandwidth (int channel) override;

        bool setRxGain (double newGain, GainElement gainElement, int channel) override;

        double getRxGain (int channel, GainElement gainElement) override;

        bool setTxCenterFrequency (double newCenterFrequency, int channel) override;

        double getTxCenterFrequency (int channel) override;

        bool setTxBandwidth (double newBandwidth, int channel) override;

        double getTxBandwidth (int channel) override;

        bool setTxGain (double newGain, GainElement gainElement, int channel) override;

        double getTxGain (int channel, GainElement gainElement) override;

    private:
        enum TreeLevel
        {
            beforeOrAfterTree,
            device,
            mboard,
            dboardOrDSP,
            frontendOrCodec
        };

        class UHDLogStreambuffer : public std::streambuf
        {
        public:
            UHDLogStreambuffer (SDRIODeviceCallback*& callbackToUse);
            int_type overflow (int_type character = EOF) override;
            std::streamsize xsputn (const char_type* string, std::streamsize count) override;
        private:
            juce::String logTempBuffer;
            SDRIODeviceCallback*& callback;
        };

        // manages the mapping between the buffer channels exposed to the public interface and the hardware ressources
        class ChannelMapping
        {
        public:
            static const juce::Identifier propertyNumChannels;
            static const juce::Identifier propertyMboardIdx;
            static const juce::Identifier propertyDboardSlot;
            static const juce::Identifier propertyFrontendOnDboard;
            static const juce::Identifier propertyAntennaPort;
            static const juce::Identifier propertyAnalogGain;
            static const juce::Identifier propertyDigitalGain;
            static const juce::Identifier propertyDigitalGainFine;
            static const juce::Identifier propertyCenterFrequency;
            static const juce::Identifier propertyAnalogBandwitdh;
            static const juce::Identifier propertyHardwareChannel;

            enum UHDGainElements : size_t {analog, digital, digitalFine, automatic, count};
            /*
            friend std::ostream& operator<< (std::ostream& os, UHDEngine::ChannelMapping::UHDGainElements gainElement)
            {
                switch (gainElement)
                {
                    case UHDEngine::ChannelMapping::UHDGainElements::analog      : return os << "analog";
                    case UHDEngine::ChannelMapping::UHDGainElements::digital     : return os << "digital";
                    case UHDEngine::ChannelMapping::UHDGainElements::digitalFine : return os << "digitalFine";
                    default : return os << static_cast<size_t>(gainElement);
                };
            }
             */

            enum Direction : juce::juce_wchar {rx = 'R', tx = 'T' };

            ChannelMapping (const juce::Array<ChannelSetup>& channelSetup, UHDEngine& engine, Direction direction);

            void setGainElements (std::vector<juce::StringArray>&& newGainElements);

            int getHardwareChannelForBufferChannel (int bufferChannel) const;

            int getMboardIdxForBufferChannel (int bufferChannel) const;

            const juce::ValueTree getMboardForBufferChannel (int bufferChannel) const;

            const juce::ValueTree getDboardForBufferChannel (int bufferChannel) const;

            const juce::ValueTree getFrontendForBufferChannel (int bufferChannel) const;

            const juce::StringArray& getSubdevSpecs() const;

            const juce::StringArray& getValidAntennas (int bufferChannel) const;

            bool isValidAntennaForChannel (const char* antennaName, int bufferChannel) const;

            juce::Result isFrontendPropertyInValidRange (const int bufferChannel, const juce::Identifier& propertyToCheck, const double valueToTest, bool updateTreeIfValid = true);

            // Returns the string of the gain element if the element exists and the gain value was valid, returns an
            // empty string if the gain element does not exist, returns a nullptr if the gain value is invalid
            const char* getGainElementStringIfGainIsInValidRange (const int bufferChannel, UHDGainElements uhdGainElement, double gainValueToCheck);

            void digitalGainPartition (const int bufferChannel, const double desiredGain, double& requiredCoarseGain, double& requiredFineGain);

            size_t* getStreamArgsChannelList();

            juce::ValueTree serializeCurrentSetup (Direction direction);
            static juce::Result deserializeSetup (juce::ValueTree& serializedSetup, UHDEngine& engine);

            const int numChannels;

        private:
            juce::Array<ChannelSetup> channelSetupHardwareOrder;
            juce::Array<size_t> bufferOrderToHardwareOrder;
            juce::Array<juce::ValueTree> mboardBufferOrder;
            juce::Array<juce::ValueTree> dboardBufferOrder;
            juce::Array<juce::ValueTree> frontendBufferOrder;
            juce::Array<juce::ValueTree> codecBufferOrder;
            juce::StringArray subdevSpecs;
            std::vector<juce::StringArray> validAntennas;
            std::vector<juce::StringArray> gainElements;
            std::vector<std::vector<juce::ValueTree>> gainElementSubtree;
            std::vector<std::array<int, UHDGainElements::count>> gainElementsMap; // Maps the gain element enum values to the strings in the gainElements array
            UHDEngine& uhdEngine;
            static const char emptyGainElementString[1];
        };

        SDRIODeviceCallback* activeCallback = nullptr;

        std::streambuf* previousClogStreambuf;
        UHDLogStreambuffer uhdLogStreambuffer;

        UHDEngine (UHDr::Ptr uhdLibrary);

        UHDr::Ptr uhdr;

        // The device tree holds all devices available, however not all devices might be used in a multi usrp setup
        juce::ValueTree deviceTree;

        juce::ValueTree devicesInActiveUSRPSetup;
        int numMboardsInDeviceTree = 0;
        juce::StringArray deviceTypesInDeviceTree;
        juce::Array<std::reference_wrapper<juce::var>> mboardsInDeviceTree;

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

        int desiredBlockSize = 1024;

        // default streaming args
        static const juce::String defaultCpuFormat;
        //static const juce::String defaultOtwFormat;
        static const juce::String emptyArg;

        juce::String lastError;

        void run() override;

        juce::ValueTree getUHDTree();

        juce::IPAddress getIPAddressForMboard (int mboardIdx);
    };

#endif //JUCE_IOS



#if !JUCE_IOS
    class UHDEngineManager : private SDRIOEngineManager
    {
        friend class SDRIOEngineManager;
    private:
        juce::String getEngineName() override;

        juce::Result isEngineAvailable() override;
#if JUCE_MODULE_AVAILABLE_juce_gui_basics
        std::unique_ptr<juce::Component> createEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints& constraints) override;
#endif

        SDRIOEngine* createEngine() override;

    private:
        UHDr::Ptr uhdr;
    };
#endif //!JUCE_IOS
}