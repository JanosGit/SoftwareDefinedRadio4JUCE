
#include "HackRfEngine.h"

namespace ntlab
{
    const juce::Identifier HackRFEngine::propertyHackRFEngine     ("HackRF_Engine");
    const juce::Identifier HackRFEngine::propertyDeviceList       ("Device_list");
    const juce::Identifier HackRFEngine::propertyHackRFConfig     ("HackRF_config");
    const juce::Identifier HackRFEngine::propertyDeviceName       ("Device_name");
    const juce::Identifier HackRFEngine::propertySampleRate       ("Sample_rate");
    const juce::Identifier HackRFEngine::propertyCenterFrequency  ("Center_frequency");
    const juce::Identifier HackRFEngine::propertyBandwidth        ("Bandwidth");
    const juce::Identifier HackRFEngine::propertyRxAnalogGain     ("Rx_analog_gain");
    const juce::Identifier HackRFEngine::propertyRxDigitalScaling ("Rx_digital_scaling");
    const juce::Identifier HackRFEngine::propertyTxAnalogGain     ("Tx_analog_gain");
    const juce::Identifier HackRFEngine::propertyTxDigitalScaling ("Tx_digital_scaling");
    const juce::Identifier HackRFEngine::propertyRxTxState        ("RxTx_state");
    const juce::Identifier HackRFEngine::propertyMaxBlockSize     ("Max_block_size");

    const uint32_t HackRFEngine::rxLNAGainMax = 40;
    const uint32_t HackRFEngine::rxLNAGainStep = 8;
    const uint32_t HackRFEngine::rxVGAGainMax = 62;
    const uint32_t HackRFEngine::rxVGAGainStep = 2;
    const uint32_t HackRFEngine::rxAnalogGainMax = rxLNAGainMax + rxVGAGainMax;
    const uint32_t HackRFEngine::txVGAGainMax = 62;
    const uint32_t HackRFEngine::txVGAGainStep = 1;

    HackRFEngine::HackRFEngine (HackRFr::Ptr hackRfLibrary) : hackrfr (hackRfLibrary)
    {
#ifdef NTLAB_FORCED_BLOCKSIZE
        // Fixed blocksize currently not implemented for HackRF
        // Fixed blocksize Approach should be removed anyway
        jassertfalse;
#endif
    }

    juce::Result HackRFEngine::selectDevice (const juce::String& deviceName)
    {
        if (currentDevice == deviceName)
            return juce::Result::ok();

        HackRFr::Error error;
        hackRF = hackrfr->createDevice (deviceName, error);

        if (error == HackRFr::Error::success)
        {
            currentDevice = deviceName;
            return juce::Result::ok();
        }

        currentDevice = "";
        hackRF.reset (nullptr);

        return juce::Result::fail ("Error creating device: " + juce::String (hackrfr->getErrorName (error)));
    }

    const int HackRFEngine::getNumRxChannels() { return 1; }
    const int HackRFEngine::getNumTxChannels() { return 1; }

    bool HackRFEngine::setSampleRate (double newSampleRate)
    {
        // make sure you have successfully selected a device
        jassert (hackRF != nullptr);

        auto error = hackRF->setSampleRate (newSampleRate);

        if (error == HackRFr::Error::success)
        {
            currentSampleRate = newSampleRate;
            return true;
        }

        currentSampleRate = 0.0;
        return false;
    }

    double HackRFEngine::getSampleRate() { return currentSampleRate; }

    juce::ValueTree HackRFEngine::getDeviceTree()
    {
        juce::ValueTree deviceTree (propertyHackRFEngine);

        auto allDevices = hackrfr->findAllDevices();

        deviceTree.setProperty (propertyDeviceList, allDevices, nullptr);

        return deviceTree;
    }

    juce::ValueTree HackRFEngine::getActiveConfig ()
    {
        juce::ValueTree activeConfig (propertyHackRFConfig);

        if (hackRF == nullptr)
        {
            activeConfig.setProperty (propertyDeviceName, "None", nullptr);
            return activeConfig;
        }

        activeConfig.setProperty (propertyDeviceName,       currentDevice,                                              nullptr);
        activeConfig.setProperty (propertySampleRate,       currentSampleRate,                                          nullptr);
        activeConfig.setProperty (propertyCenterFrequency,  static_cast<int64_t> (currentCenterFrequency),              nullptr);
        activeConfig.setProperty (propertyBandwidth,        static_cast<int64_t> (currentBandwidth),                    nullptr);
        activeConfig.setProperty (propertyRxAnalogGain,     static_cast<int64_t> (currentRxLNAGain + currentRxVGAGain), nullptr);
        activeConfig.setProperty (propertyRxDigitalScaling, currentRxDigitalScaling,                                    nullptr);
        activeConfig.setProperty (propertyTxAnalogGain,     static_cast<int64_t> (currentTxVGAGain),                    nullptr);
        activeConfig.setProperty (propertyTxDigitalScaling, currentTxDigitalScaling,                                    nullptr);
        activeConfig.setProperty (propertyRxTxState,        static_cast<int> (rxTxState),                               nullptr);
        activeConfig.setProperty (propertyMaxBlockSize,     maxBufferSize,                                              nullptr);

        return activeConfig;
    }

    juce::Result HackRFEngine::setConfig (juce::ValueTree& configToSet)
    {
        if (!configToSet.hasType (propertyHackRFConfig))
            return juce::Result::fail ("Expecting a config of type " + propertyHackRFConfig.toString() + " but got a config of type " + configToSet.getType().toString());

        juce::String deviceName = configToSet.getProperty (propertyDeviceName);

        auto selectingDeviceResult = selectDevice (deviceName);

        if (selectingDeviceResult.failed())
            return selectingDeviceResult;

        if (!setSampleRate (configToSet.getProperty (propertySampleRate)))
            return juce::Result::fail ("Error setting sample rate of " + configToSet.getProperty (propertySampleRate).toString() + "Hz");

        if (!setCenterFrequency (configToSet.getProperty (propertyCenterFrequency)))
            return juce::Result::fail ("Error setting center frequency of " + configToSet.getProperty (propertyCenterFrequency).toString() + "Hz");

        if (double bandwidth = configToSet.getProperty (propertyBandwidth))
        {
            if (!setBandwidth (bandwidth))
                return juce::Result::fail ("Error setting bandwidth of " + configToSet.getProperty (propertyBandwidth).toString() + "Hz");
        }

        if (!setRxGain (configToSet.getProperty (propertyRxAnalogGain), GainElement::analog, 0))
            return juce::Result::fail ("Error setting Rx Gain of " + configToSet.getProperty (propertyRxAnalogGain).toString() + "dB");

        currentRxDigitalScaling = configToSet.getProperty (propertyRxDigitalScaling);

        if (!setTxGain (configToSet.getProperty (propertyTxAnalogGain), GainElement::analog, 0))
            return juce::Result::fail ("Error setting Tx Gain of " + configToSet.getProperty (propertyTxAnalogGain).toString() + "dB");

        currentTxDigitalScaling = configToSet.getProperty (propertyTxDigitalScaling);

        if (!enableRxTx (static_cast<RxTxState> (static_cast<int> (configToSet.getProperty (propertyRxTxState)))))
            return juce::Result::fail ("Error setting Rx/Tx state");

        if (!setDesiredBlockSize (configToSet.getProperty (propertyMaxBlockSize)))
            return juce::Result::fail ("Error setting max block size of " + configToSet.getProperty (propertyMaxBlockSize).toString());

        return juce::Result::ok();
    }

    bool HackRFEngine::setDesiredBlockSize (int desiredBlockSize)
    {
        if (desiredBlockSize == maxBufferSize)
            return true;

        return false;
    }

    bool HackRFEngine::isReadyToStream ()
    {
        if ((hackRF != nullptr) &&
            (currentSampleRate > 0.0))
            return true;

        return false;
    }

    bool HackRFEngine::startStreaming (ntlab::SDRIODeviceCallback* callback)
    {
        if (!isReadyToStream())
            return false;

        if (isStreaming())
            return true;

        // an old callback is waiting to finish
        jassert (currentCallback == nullptr);

        if (currentCallback != nullptr)
            return false;

        currentCallback = callback;

        startStopThread.addJob ([this] ()
        {
#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
            rxBuffer.reset (new CLSampleBufferComplex<float> (1, maxBufferSize, clQueue, clContext, false, CL_MEM_READ_ONLY, CL_MAP_WRITE));
            txBuffer.reset (new CLSampleBufferComplex<float> (1, maxBufferSize, clQueue, clContext, false, CL_MEM_WRITE_ONLY, CL_MAP_READ));
#else
            rxBuffer.reset (new SampleBufferComplex<float> (1, maxBufferSize));
            txBuffer.reset (new SampleBufferComplex<float> (1, maxBufferSize));
#endif

            currentCallback->prepareForStreaming (currentSampleRate, 1, 1, maxBufferSize);

            if (rxTxState == rxEnabled)
                hackRF->startRx (rxCallback, this);
            else
                hackRF->startTx (txCallback, this);
        });

        return true;
    }

    void HackRFEngine::stopStreaming ()
    {
        if (isStreaming())
        {
            if (rxTxState == rxEnabled)
                hackRF->stopRx();
            else
                hackRF->stopTx();

            startStopThread.addJob ([this]()
            {
                const int timeoutMillisecs = 200;

                int timeWaited = 0;

                while (isStreaming())
                {
                    timeWaited += 50;

                    if (timeWaited > timeoutMillisecs)
                    {
                        // the device does not seem to react when trying to stop it...
                        jassertfalse;

                        currentCallback->handleError (GeneralErrorMessageNonOwning ("Stopping stream by force, additional false processing callbacks might occur"));
                        break;
                    }

                    juce::Thread::sleep (50);
                }

                currentCallback->streamingHasStopped();
                currentCallback = nullptr;
            });

        }
    }

    bool HackRFEngine::isStreaming ()
    {
        if (hackRF == nullptr)
            return false;

        return hackRF->isStreaming();
    }

    bool HackRFEngine::enableRxTx (RxTxState newRxTxState)
    {
        // HackRF only supports half-duplex
        jassert (newRxTxState != RxTxState::rxTxEnabled);

        if (newRxTxState == RxTxState::rxTxEnabled)
            return false;

        rxTxState = newRxTxState;

        return true;
    }

    bool HackRFEngine::setRxCenterFrequency (double newCenterFrequency, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return setCenterFrequency (newCenterFrequency);
    }

    double HackRFEngine::getRxCenterFrequency (int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return currentCenterFrequency;
    }

    bool HackRFEngine::setRxBandwidth (double newBandwidth, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return setBandwidth (newBandwidth);
    }

    double HackRFEngine::getRxBandwidth (int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return currentBandwidth;
    }

    bool HackRFEngine::setRxGain (double newGain, GainElement gainElement, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel

        switch (gainElement)
        {
            case analog:
            {
                // make sure you have successfully selected a device
                jassert (hackRF != nullptr);

                // Trying to do as much as possible with LNA gain stage
                currentRxLNAGain = std::min (static_cast<uint32_t> (newGain), rxLNAGainMax);
                currentRxLNAGain -= currentRxLNAGain % rxLNAGainStep;

                auto error = hackRF->setLNAGain (currentRxLNAGain);
                if (error != HackRFr::Error::success)
                    return false;

                // Computing remaining gain
                newGain -= currentRxLNAGain;

                currentRxVGAGain = std::min (static_cast<uint32_t> (newGain), rxVGAGainMax);
                currentRxVGAGain -= currentRxVGAGain & rxVGAGainStep;

                error = hackRF->setVGAGain (currentRxVGAGain);
                if (error != HackRFr::Error::success)
                    return false;

                newGain -= currentRxVGAGain;

                // The gain you applied could not be handled by the analog gain stages. Try setting an unspecified gain
                // element instead, this will lead to a digital makeup for the rest of the gain
                jassert (newGain < rxVGAGainStep);

                return true;
            }

            case digital:
                // convert to linear scaling value
                currentRxDigitalScaling = static_cast<float> (std::pow (10.0, newGain * 0.1));
                return true;

            default:
            {
                // Trying to do as much as possible with analog gain stages
                double desiredAnalogGain = std::min (newGain, static_cast<double> (rxAnalogGainMax));

                bool success = setRxGain (desiredAnalogGain, analog, 0);
                if (!success)
                    return false;

                double trueAnalogGain = getRxGain (0, analog);

                newGain -= trueAnalogGain;
                if (newGain != 0)
                    success = setRxGain (newGain, digital, 0);

                return success;
            }
        }
    }

    double HackRFEngine::getRxGain (int channel, ntlab::SDRIOHardwareEngine::GainElement gainElement)
    {
        jassert (channel < 1); // HackRF only has 1 channel

        switch (gainElement)
        {
            case analog:
                return currentRxLNAGain + currentRxVGAGain;
            case digital:
                return std::log10 (currentRxDigitalScaling) * 10.0;
            default:
                return getRxGain (0, analog) + getRxGain (0, digital);
        }
    }

    bool HackRFEngine::setTxCenterFrequency (double newCenterFrequency, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return setCenterFrequency (newCenterFrequency);
    }

    double HackRFEngine::getTxCenterFrequency (int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return currentCenterFrequency;
    }

    bool HackRFEngine::setTxBandwidth (double newBandwidth, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return setBandwidth (newBandwidth);
    }

    double HackRFEngine::getTxBandwidth (int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel
        return currentBandwidth;
    }
    
    bool HackRFEngine::setTxGain (double newGain, ntlab::SDRIOHardwareEngine::GainElement gainElement, int channel)
    {
        jassert (channel < 1); // HackRF only has 1 channel

        switch (gainElement)
        {
            case analog:
            {
                // make sure you have successfully selected a device
                jassert (hackRF != nullptr);

                currentTxVGAGain = std::min (static_cast<uint32_t> (newGain), txVGAGainMax);

                auto error = hackRF->setTxVGAGain (currentTxVGAGain);
                if (error != HackRFr::Error::success)
                    return false;

                newGain -= currentTxVGAGain;

                // The gain you applied could not be handled by the analog gain stages. Try setting an unspecified gain
                // element instead, this will lead to a digital makeup for the rest of the gain
                jassert (newGain < txVGAGainStep);

                return true;
            }

            case digital:
                // convert to linear scaling value
                currentTxDigitalScaling = static_cast<float> (std::pow (10.0, newGain * 0.1));
                return true;

            default:
            {
                // Trying to do as much as possible with analog gain stages
                double desiredAnalogGain = std::min (newGain, static_cast<double> (txVGAGainMax));

                bool success = setTxGain (desiredAnalogGain, analog, 0);
                if (!success)
                    return false;

                double trueAnalogGain = getTxGain (0, analog);

                newGain -= trueAnalogGain;
                if (newGain != 0)
                    success = setTxGain (newGain, digital, 0);

                return success;
            }
        }
    }

    double HackRFEngine::getTxGain (int channel, ntlab::SDRIOHardwareEngine::GainElement gainElement)
    {
        jassert (channel < 1); // HackRF only has 1 channel

        switch (gainElement)
        {
            case analog:
                return currentTxVGAGain;
            case digital:
                return std::log10 (currentTxDigitalScaling) * 10.0;
            default:
                return getTxGain (0, analog) + getRxGain (0, digital);
        }
    }

    int HackRFEngine::rxCallback (ntlab::HackRFr::Transfer* transfer)
    {
        auto* engine = static_cast<HackRFEngine*> (transfer->rxContext);

        // please report a github issue if this assert is hit
        jassert (transfer->validLength <= (2 * engine->maxBufferSize));

        const float scaleFactor = engine->currentRxDigitalScaling * (1.0f / std::numeric_limits<int8_t>::max());

        float*        rxBuffer       = reinterpret_cast<float*> (engine->rxBuffer->getWritePointer (0));
        const int8_t* transferBuffer = transfer->buffer;

        // convert to float
        for (int sample = 0; sample < transfer->validLength; ++sample)
            rxBuffer[sample] = transferBuffer[sample] * scaleFactor - 1.0f;

        engine->rxBuffer->setNumSamples (transfer->validLength / 2);
        engine->txBuffer->setNumSamples (0);

        engine->currentCallback->processRFSampleBlock (*engine->rxBuffer, *engine->txBuffer);

        if (engine->rxTxState == txEnabled)
        {
            engine->hackRF->stopRx();
            engine->hackRF->startTx (txCallback, engine);
        }

        return HackRFr::Error::success;
    }

    int HackRFEngine::txCallback (ntlab::HackRFr::Transfer* transfer)
    {
        auto* engine = static_cast<HackRFEngine*> (transfer->txContext);

        // please report a github issue if this assert is hit
        jassert (transfer->validLength <= (2 * engine->maxBufferSize));

        const float scaleFactor = engine->currentTxDigitalScaling * std::numeric_limits<int8_t>::max();

        engine->rxBuffer->setNumSamples (0);
        engine->txBuffer->setNumSamples (transfer->validLength / 2);

        engine->currentCallback->processRFSampleBlock (*engine->rxBuffer, *engine->txBuffer);

        auto* transferBuffer = transfer->buffer;
        auto* txBuffer       = reinterpret_cast<const float*> (engine->txBuffer->getReadPointer (0));

        for (int sample = 0; sample < transfer->validLength; ++sample)
        {
            float scaledSample = scaleFactor * txBuffer[sample];

            // overflow!! lower the digital gain
            jassert ((scaledSample <= std::numeric_limits<int8_t>::max()) && (scaledSample >= std::numeric_limits<int8_t>::min()));

            transferBuffer[sample] = static_cast<int8_t> (scaledSample);
        }

        if (engine->rxTxState == rxEnabled)
        {
            engine->hackRF->stopTx();
            engine->hackRF->startRx (rxCallback, engine);
        }

        return HackRFr::Error::success;
    }

    bool HackRFEngine::setCenterFrequency (double newCenterFrequency)
    {
        // make sure you have successfully selected a device
        jassert (hackRF != nullptr);

        jassert (newCenterFrequency > 0.0);

        auto error = hackRF->setFreq (static_cast<uint64_t> (newCenterFrequency));

        if (error == HackRFr::Error::success)
        {
            currentCenterFrequency = static_cast<uint64_t> (newCenterFrequency);
            return true;
        }

        return false;
    }

    bool HackRFEngine::setBandwidth (double newBandwidth)
    {
        // make sure you have successfully selected a device
        jassert (hackRF != nullptr);

        jassert (newBandwidth > 0.0);

        // aliasing alert!!
        jassert (newBandwidth <= getSampleRate());

        auto error = hackRF->setBasebandFilterBandwidth (static_cast<uint32_t> (newBandwidth));

        if (error == HackRFr::Error::success)
        {
            currentBandwidth = static_cast<uint32_t> (newBandwidth);
            return true;
        }

        return false;
    }

#if !JUCE_IOS

    juce::String HackRFEngineManager::getEngineName ()
    {
        return "HackRF Engine";
    }

    juce::Result HackRFEngineManager::isEngineAvailable ()
    {
        if (hackrfr != nullptr)
            return juce::Result::ok();

        juce::DynamicLibrary hackrfLib;
        if (hackrfLib.open (HackRFr::hackRFLibName))
        {
            hackrfLib.close();

            juce::String error;
            hackrfr = HackRFr::load (HackRFr::hackRFLibName, error);

            if (hackrfr == nullptr)
                return juce::Result::fail (error);

            return juce::Result::ok();
        }

        return juce::Result::fail (HackRFr::hackRFLibName + " cannot be found on this system");
    }

    SDRIOEngine* HackRFEngineManager::createEngine ()
    {
        return new HackRFEngine (hackrfr);
    }

#endif //!JUCE_IOS
}