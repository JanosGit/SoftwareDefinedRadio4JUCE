

#pragma once

#include "../SDRIOEngine.h"
#include "HackRFReplacement.h"

namespace ntlab
{

    class HackRFEngine
#if !JUCE_IOS
            : public SDRIOHardwareEngine
    {
        friend class HackRFEngineManager;
        friend class ChannelMapping;
        friend class UHDEngineConfigurationComponent;
#else
    {
#endif
    public:

        static const juce::Identifier propertyHackRFEngine;
        static const juce::Identifier propertyDeviceList;
        static const juce::Identifier propertyHackRFConfig;
        static const juce::Identifier propertyDeviceName;
        static const juce::Identifier propertySampleRate;
        static const juce::Identifier propertyCenterFrequency;
        static const juce::Identifier propertyBandwidth;
        static const juce::Identifier propertyRxAnalogGain;
        static const juce::Identifier propertyRxDigitalScaling;
        static const juce::Identifier propertyTxAnalogGain;
        static const juce::Identifier propertyTxDigitalScaling;
        static const juce::Identifier propertyRxTxState;
        static const juce::Identifier propertyMaxBlockSize;

        /**
         * Must be called before any settings are applied to select a device to operate with.
         * The deviceName must be a device name retrieved by getDeviceTree, e.g. like that
         *
         * @code

           ntlab::SDRIODeviceManager deviceManager;

           deviceManager.addDefaultEngines();

           if (deviceManager.selectEngine("HackRF Engine"))
           {
              auto& hackRf = dynamic_cast<ntlab::HackRFEngine&> (*deviceManager.getSelectedEngine());

              auto tree = hackRf.getDeviceTree();

              auto allDeviceNames = tree[propertyDeviceName];
              int  numDevices     = allDeviceNames.size();

              for (int d = 0; d < numDevices; ++d)
                  std::cout << allDeviceNames[d].toString() << std::endl;

              // simply selecting the first device
              auto selectingDeviceResult = selectDevice (allDeviceNames[0]);

              if (selectingDeviceResult.failed())
                  std::cerr << selectingDeviceResult
           }

         * @endcode
         */
        juce::Result selectDevice (const juce::String& deviceName);

        const int getNumRxChannels() override;

        const int getNumTxChannels() override;

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

        HackRFr::Ptr hackrfr;
        std::unique_ptr<HackRFr::HackRF> hackRF;
        juce::String currentDevice;

        double   currentSampleRate      = 0.0;
        uint64_t currentCenterFrequency = 0;
        uint32_t currentBandwidth       = 0;
        uint32_t currentRxLNAGain       = 0;
        uint32_t currentRxVGAGain       = 0;
        float currentRxDigitalScaling   = 1.0f;
        uint32_t currentTxVGAGain       = 0;
        float currentTxDigitalScaling   = 1.0f;
        RxTxState rxTxState = rxEnabled;

        static const uint32_t rxLNAGainMax;
        static const uint32_t rxLNAGainStep;
        static const uint32_t rxVGAGainMax;
        static const uint32_t rxVGAGainStep;
        static const uint32_t rxAnalogGainMax;
        static const uint32_t txVGAGainMax;
        static const uint32_t txVGAGainStep;

        SDRIODeviceCallback* currentCallback = nullptr;

        static const int maxBufferSize = 131072; // this seems to be a constant value the api uses
        std::unique_ptr<OptionalCLSampleBufferComplexFloat> rxBuffer, txBuffer;

        juce::ThreadPool startStopThread {1};

        HackRFEngine (HackRFr::Ptr hackRfLibrary);

        static int rxCallback (HackRFr::Transfer* transfer);

        static int txCallback (HackRFr::Transfer* transfer);

        bool setCenterFrequency (double newCenterFrequency);

        bool setBandwidth (double newBandwidth);

    };

#if !JUCE_IOS
    class HackRFEngineManager : private SDRIOEngineManager
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
        HackRFr::Ptr hackrfr;
    };
#endif //!JUCE_IOS
}