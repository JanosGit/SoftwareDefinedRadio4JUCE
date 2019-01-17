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

#include "UHDEngine.h"
#include "UHDUSRPProbeParser.h"
#include "../../ErrorHandling/ErrorHandlingMacros.h"

#include <typeinfo> // for std::bad_cast

namespace ntlab
{

// ugly solution but makes the NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR macro work
#define errorDescription UHDr::errorDescription

    const juce::String UHDEngine::defaultCpuFormat ("fc32");
    const juce::String UHDEngine::defaultOtwFormat ("sc16");
    const juce::String UHDEngine::defaultArgs;

    const juce::Identifier UHDEngine::propertyUSRPDevice ("USRP Device");
    const juce::Identifier UHDEngine::propertyIPAddress  ("ip-addr");

    UHDEngine* UHDEngine::castToUHDEnginePtr (std::unique_ptr<ntlab::SDRIOEngine>& baseEnginePtr) { return dynamic_cast<ntlab::UHDEngine*> (baseEnginePtr.get()); }

    UHDEngine& UHDEngine::castToUHDEngineRef (std::unique_ptr<ntlab::SDRIOEngine>& baseEnginePtr)
    {
        auto uhdEnginePtr = castToUHDEnginePtr (baseEnginePtr);
        if (uhdEnginePtr == nullptr)
            throw std::bad_cast();

        return *uhdEnginePtr;
    }

    UHDEngine::~UHDEngine ()
    {
        if (threadShouldExit())
        {
            waitForThreadToExit (2100);
        }
        else
        {
            stopThread (2000);
        }
    }

    UHDEngine::UHDEngine (ntlab::UHDr::Ptr uhdLibrary)
      : juce::Thread ("UHD Engine Thread"),
        uhdr (uhdLibrary) {}

    juce::Result UHDEngine::makeUSRP (juce::StringPairArray& args, SynchronizationSetup synchronizationSetup)
    {
        // todo: Resize tune result arrays
        UHDr::Error error;
        usrp = uhdr->makeUSRP (args, error);

        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        numMboardsInUSRP = usrp->getNumMboards();

#if JUCE_DEBUG

        if (synchronizationSetup == singleDeviceStandalone)
        {
            // you cannot use a single device clocking setup if you have more than one device in your setup
            jassert (numMboardsInUSRP == 1);
        }
        if (synchronizationSetup == twoDevicesMIMOCableMasterSlave)
        {
            //todo: find out if mboards have mimo port??
            // you cannot use a mimo setup with more or less than two devices
            jassert (numMboardsInUSRP == 2);
        }
#endif

        this->synchronizationSetup = synchronizationSetup;

        return juce::Result::ok();
    }

    juce::Result UHDEngine::makeUSRP (juce::Array<juce::IPAddress> ipAddresses, SynchronizationSetup synchronizationSetup)
    {
        juce::StringPairArray args;
        for (int i = 0; i < ipAddresses.size(); ++i)
        {
            if (ipAddresses[i].isIPv6)
                return juce::Result::fail ("Only IPv4 addresses supported");

            args.set ("addr" + juce::String (i), ipAddresses[i].toString());
        }

        return makeUSRP (args, synchronizationSetup);

    }

    juce::Result UHDEngine::setupRxChannels (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup)
    {
        if (numMboardsInUSRP == 0)
            return juce::Result::fail ("No motherboards found");

        rxChannelMapping.reset (new ChannelMapping (channelSetup, *this));
        auto subdevSpecs = rxChannelMapping->getSubdevSpecs();

        for (int m = 0; m < numMboardsInUSRP; ++m)
        {
            auto setRxSubdevSpec = usrp->setRxSubdevSpec (subdevSpecs[m], m);
            if (setRxSubdevSpec.failed())
                return setRxSubdevSpec;
        }

        UHDr::Error error;
        UHDr::StreamArgs streamArgs;
        streamArgs.numChannels = rxChannelMapping->numChannels;
        streamArgs.channelList = rxChannelMapping->getStreamArgsChannelList();
        streamArgs.cpuFormat   = const_cast<char*> (defaultCpuFormat.toRawUTF8());
        streamArgs.otwFormat   = const_cast<char*> (defaultOtwFormat.toRawUTF8());
        streamArgs.args        = const_cast<char*> (defaultArgs.toRawUTF8());
        rxStream.reset (usrp->makeRxStream (streamArgs, error));

        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, rxStream.reset (nullptr);)

        for (int c = 0; c < rxChannelMapping->numChannels; ++c)
        {
            auto setRxAntenna = usrp->setRxAntenna (channelSetup[c].antennaPort.toRawUTF8(), c);
            if (setRxAntenna.failed())
                return setRxAntenna;
        }

        rxEnabled = true;

        return juce::Result::ok();
    }

    juce::Result UHDEngine::setupTxChannels (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup)
    {
        if (numMboardsInUSRP == 0)
            return juce::Result::fail ("No motherboards found");

        txChannelMapping.reset (new ChannelMapping (channelSetup, *this));
        auto subdevSpecs = txChannelMapping->getSubdevSpecs();

        for (int m = 0; m < numMboardsInUSRP; ++m)
        {
            auto setTxSubdevSpec = usrp->setTxSubdevSpec (subdevSpecs[m], m);
            if (setTxSubdevSpec.failed())
                return setTxSubdevSpec;
        }

        UHDr::Error error;
        UHDr::StreamArgs streamArgs;
        streamArgs.numChannels = txChannelMapping->numChannels;
        streamArgs.channelList = txChannelMapping->getStreamArgsChannelList();
        streamArgs.cpuFormat   = const_cast<char*> (defaultCpuFormat.toRawUTF8());
        streamArgs.otwFormat   = const_cast<char*> (defaultOtwFormat.toRawUTF8());
        streamArgs.args        = const_cast<char*> (defaultArgs.toRawUTF8());

        txStream.reset (usrp->makeTxStream (streamArgs, error));

        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, txStream.reset (nullptr);)


        for (int c = 0; c < txChannelMapping->numChannels; ++c)
        {
            auto setTxAntenna = usrp->setTxAntenna (channelSetup[c].antennaPort.toRawUTF8(), c);
            if (setTxAntenna.failed())
                return setTxAntenna;
        }

        txEnabled = true;

        return juce::Result::ok();
    }

    bool UHDEngine::setSampleRate (double newSampleRate)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        auto setSampleRate = usrp->setRxSampleRate (newSampleRate, allChannels);
        if (setSampleRate.failed())
        {
            lastError = setSampleRate.getErrorMessage();
            return false;
        }

        setSampleRate = usrp->setTxSampleRate (newSampleRate, allChannels);

        NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setSampleRate);
    }

    double UHDEngine::getSampleRate ()
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double rxSampleRate = usrp->getRxSampleRate (0, error);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED (error);

        double txSampleRate = usrp->getTxSampleRate (0, error);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED (error);

        if (rxSampleRate != txSampleRate)
        {
            lastError = "Error getting sample rate, different samplerates for rx (" + juce::String (rxSampleRate) + "Hz) and tx (" + juce::String (txSampleRate) + ") returned";
            DBG (lastError);
            return -1.0;
        }

        return txSampleRate;
    }

    const juce::var& UHDEngine::getDeviceTree()
    {
        deviceTree = parseUHDUSRPProbe (uhdr);

        numMboardsInDeviceTree = 0;
        mboardsInDeviceTree.clearQuick();
        deviceTypesInDeviceTree.clearQuick();

        auto devices = deviceTree.getProperty (propertyUSRPDevice, juce::var()).getDynamicObject()->getProperties();

        for (auto& device : devices)
        {
            auto mboards = device.value.getDynamicObject()->getProperties();
            numMboardsInDeviceTree += mboards.size();

            for (auto& mboard : mboards)
            {
                auto mboardProperty = mboard.value.getDynamicObject()->getProperties();
                mboardsInDeviceTree.add (const_cast<juce::var&> (mboardProperty.getValueAt (0)));
                deviceTypesInDeviceTree.add (mboardProperty.getName (0).toString());
            }
        }

        return deviceTree;
    }

    const juce::var& UHDEngine::getDeviceTreeUpdateIfNotPresent ()
    {
        if (deviceTree.isVoid())
            getDeviceTree();

        return deviceTree;
    }

    juce::var UHDEngine::getActiveConfig ()
    {
        return juce::var();
    }

    bool UHDEngine::setConfig (juce::var& configToSet)
    {
        return true;
    }

    bool UHDEngine::setDesiredBlockSize (int desiredBlockSize)
    {
        this->desiredBlockSize = desiredBlockSize;
        return true;
    }

    bool UHDEngine::isReadyToStream ()
    {
        return ((rxStream != nullptr) || (txStream != nullptr));
    }

    bool UHDEngine::startStreaming (ntlab::SDRIODeviceCallback* callback)
    {
        if (!isReadyToStream())
            return false;

        activeCallback = callback;
        startThread (realtimeAudioPriority);

       return true;
    }

    void UHDEngine::stopStreaming ()
    {
        stopThread (2000);
    }

    bool UHDEngine::isStreaming ()
    {
        return isThreadRunning();
    }

    bool UHDEngine::enableRxTx (bool enableRx, bool enableTx)
    {
        if (!(enableRx || enableTx))
            return false;

        if (enableRx && (rxStream == nullptr))
            return false;

        if (enableTx && (txStream == nullptr))
            return false;

        rxEnabled = enableRx;
        txEnabled = enableTx;

        return true;
    }

    bool UHDEngine::setRxCenterFrequency (double newCenterFrequency, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        UHDr::TuneRequest request;
        char requestArgs[] = "";
        request.targetFreq = newCenterFrequency;
        request.args = requestArgs;

        juce::Result setRxFrequency = juce::Result::ok();

        if (channel == allChannels)
        {
            setRxFrequency = usrp->setRxFrequency (request, rxTuneResults, channel);
        }
        else
        {
            //todo: set with channel mapping
            jassertfalse;
            return false;
        }

        if (setRxFrequency.failed())
        {
            DBG (lastError = setRxFrequency.getErrorMessage());
            return false;
        }

        for (auto& result : rxTuneResults)
        {
            //if (!((result.actualRfFreq == result.targetRfFreq) && (result.actualDspFreq == result.targetDspFreq)))
            if (!juce::approximatelyEqual (result.actualRfFreq, result.targetRfFreq))
            {
                DBG (lastError = "Error setting exact Rx center frequency. Target Rf frequency: "
                        + juce::String (result.targetRfFreq) + "Hz, actual Rf frequency: "
                        + juce::String (result.actualRfFreq) + "Hz, target DSP frequency: "
                        + juce::String (result.targetDspFreq) + "Hz, actual DSP frequency: "
                        + juce::String (result.actualDspFreq) + "Hz");
                return false;
            }
        }

        return true;
    }

    double UHDEngine::getRxCenterFrequency (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double rxFreq = usrp->getRxFrequency (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, rxFreq);
    }

    bool UHDEngine::setRxBandwidth (double newBandwidth, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        auto setRxBandwidth = usrp->setRxBandwidth (newBandwidth, channel);

        NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setRxBandwidth);
    }

    double UHDEngine::getRxBandwidth (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double rxBandwidth = usrp->getRxBandwidth (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, rxBandwidth);
    }

    bool UHDEngine::setRxGain (double newGain, ntlab::SDRIOHardwareEngine::GainElement gainElement, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        // todo: currently the gain element is not mapped to any UHD compatible gain element string. Find out if there is a proper mapping
        const char* gainElementString = "";
        auto setRxGain = usrp->setRxGain (newGain, channel, gainElementString);

        NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setRxGain);
    }

    double UHDEngine::getRxGain (int channel, ntlab::SDRIOHardwareEngine::GainElement gainElement)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        // todo: currently the gain element is not mapped to any UHD compatible gain element string. Find out if there is a proper mapping
        const char* gainElementString = "";
        UHDr::Error error;
        double rxGain = usrp->getRxGain (channel, error, gainElementString);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, rxGain);
    }

    bool UHDEngine::setTxCenterFrequency (double newCenterFrequency, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        UHDr::TuneRequest request;
        char requestArgs[] = "";
        request.targetFreq = newCenterFrequency;
        request.args = requestArgs;

        txTuneResults.clearQuick();

        auto setTxFrequency = usrp->setTxFrequency (request, txTuneResults, channel);

        if (setTxFrequency.failed())
        {
            DBG (lastError = setTxFrequency.getErrorMessage());
            return false;
        }

        for (auto& result : txTuneResults)
        {
            //if (!((result.actualRfFreq == result.targetRfFreq) && (result.actualDspFreq == result.targetDspFreq)))
            if (!juce::approximatelyEqual (result.actualRfFreq, result.targetRfFreq))
            {
                DBG (lastError = "Error setting exact Tx center frequency. Target Rf frequency: "
                        + juce::String (result.targetRfFreq)  + ", actual Rf frequency: "
                        + juce::String (result.actualRfFreq)  + ", target DSP frequency: "
                        + juce::String (result.targetDspFreq) + ", actual DSP frequency: "
                        + juce::String (result.actualDspFreq));
                return false;
            }
        }

        return true;
    }

    double UHDEngine::getTxCenterFrequency (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double txFrequency = usrp->getTxFrequency (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txFrequency);
    }

    bool UHDEngine::setTxBandwidth (double newBandwidth, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        auto setTxBandwidth = usrp->setTxBandwidth (newBandwidth, channel);

        NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setTxBandwidth);
    }

    double UHDEngine::getTxBandwidth (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double txBandwidth = usrp->getTxBandwidth (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txBandwidth);
    }

    bool UHDEngine::setTxGain (double newGain, ntlab::SDRIOHardwareEngine::GainElement gainElement, int channel)
    {
        NTLAB_RETURN_FALSE_IF (usrp == nullptr);

        // todo: currently the gain element is not mapped to any UHD compatible gain element string. Find out if there is a proper mapping
        const char* gainElementString = "";
        auto setTxGain = usrp->setTxGain (newGain, channel, gainElementString);

        NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setTxGain);
    }

    double UHDEngine::getTxGain (int channel, ntlab::SDRIOHardwareEngine::GainElement gainElement)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        // todo: currently the gain element is not mapped to any UHD compatible gain element string. Find out if there is a proper mapping
        const char* gainElementString = "";
        UHDr::Error error;
        double txGain = usrp->getTxGain (channel, error, gainElementString);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txGain);
    }

    UHDEngine::ChannelMapping::ChannelMapping (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup, UHDEngine& engine)
      : channelSetupHardwareOrder (channelSetup),
        numChannels (channelSetup.size()),
        uhdEngine (engine)
    {
        int numChannels = channelSetup.size();
        size_t hardwareChannel = 0;
        bufferOrderToHardwareOrder.resize (numChannels);

        for (int m = 0; m < engine.numMboardsInUSRP; ++m)
        {
            juce::String motherboardSubdevSpec;

            for (int c = 0; c < numChannels; ++c)
            {
                // if this assert is hit one of the mboardIdx values was out of range
                jassert (juce::isPositiveAndBelow (channelSetup[c].mboardIdx, engine.numMboardsInUSRP));
                if (channelSetup[c].mboardIdx == m)
                {
                    motherboardSubdevSpec += channelSetup[c].daughterboardSlot + ":" + channelSetup[c].frontendOnDaughterboard + " ";
                    bufferOrderToHardwareOrder.set (c, hardwareChannel);
                    ++hardwareChannel;
                }
            }

            motherboardSubdevSpec.trimEnd();
            subdevSpecs.add (motherboardSubdevSpec);
        }
    }

    int UHDEngine::ChannelMapping::getMboardForBufferChannel (int bufferChannel) {return channelSetupHardwareOrder[static_cast<int> (bufferOrderToHardwareOrder[bufferChannel])].mboardIdx; }

    const juce::String& UHDEngine::ChannelMapping::getDboardForBufferChannel (int bufferChannel) {return channelSetupHardwareOrder[static_cast<int> (bufferOrderToHardwareOrder[bufferChannel])].daughterboardSlot; }

    const juce::String& UHDEngine::ChannelMapping::getFrontendForBufferChannel (int bufferChannel) {return channelSetupHardwareOrder[static_cast<int> (bufferOrderToHardwareOrder[bufferChannel])].frontendOnDaughterboard; }

    const juce::StringArray& UHDEngine::ChannelMapping::getSubdevSpecs () {return subdevSpecs; }

    size_t* UHDEngine::ChannelMapping::getStreamArgsChannelList () {return bufferOrderToHardwareOrder.data(); }

    void UHDEngine::run()
    {
        usrp->setRealtimeThreadID (juce::Thread::getCurrentThreadId());

        std::unique_ptr<OptionalCLSampleBufferComplexFloat> rxBuffer;
        std::unique_ptr<OptionalCLSampleBufferComplexFloat> txBuffer;

        int numRxChannels = 0;
        int numTxChannels = 0;
        if (rxChannelMapping != nullptr)
            numRxChannels = rxChannelMapping->numChannels;
        if (txChannelMapping != nullptr)
            numTxChannels = txChannelMapping->numChannels;

        int maxBlockSize = desiredBlockSize;

        if (rxStream != nullptr)
            maxBlockSize = std::min (maxBlockSize, rxStream->getMaxNumSamplesPerBlock());

        if (txStream != nullptr)
            maxBlockSize = std::min (maxBlockSize, txStream->getMaxNumSamplesPerBlock());

        activeCallback->prepareForStreaming (getSampleRate(), numRxChannels, numTxChannels, maxBlockSize);

        if (rxStream != nullptr)
            rxBuffer.reset (new OptionalCLSampleBufferComplexFloat (numRxChannels, maxBlockSize));
        else
            rxBuffer.reset (new OptionalCLSampleBufferComplexFloat (0, 0));

        if (txStream != nullptr)
            txBuffer.reset (new OptionalCLSampleBufferComplexFloat (numTxChannels, maxBlockSize));
        else
            txBuffer.reset (new OptionalCLSampleBufferComplexFloat (0, 0));

        switch (synchronizationSetup)
        {
            case singleDeviceStandalone:
                usrp->setTimeNow (0, 0.0, 0);
                break;
            case externalSyncAndClock:
                // reset the devices timestamp, this will lead the following stream commands to be executed in parallel
                // exactly one second after this call. This should be enough time for the hardware to prepare
                usrp->setTimeUnknownPPS (0, 0);
            case twoDevicesMIMOCableMasterSlave:
                // the master device
                usrp->setTimeNow (0, 0.0, 0);

                // the slave device
                usrp->setClockSource ("mimo", 1);
                usrp->setTimeSource  ("mimo", 1);

                //sleep a bit while the slave locks its time to the master
                sleep (100);
        }

        const time_t delayInSecondsUntilStreamingStarts = 1;


        if (rxStream != nullptr)
        {
            UHDr::StreamCmd streamCmd;
            streamCmd.numSamples = static_cast<size_t> (maxBlockSize);
            streamCmd.streamMode = UHDr::StreamMode::startContinuous;
            streamCmd.streamNow = false;
            streamCmd.timeSpecFracSecs = 0;
            streamCmd.timeSpecFullSecs = delayInSecondsUntilStreamingStarts;

            juce::Result issueStreamCmd = rxStream->issueStreamCmd (streamCmd);
            if (issueStreamCmd.failed())
            {
                activeCallback->handleError (issueStreamCmd.getErrorMessage() + ". Stopping stream.");
                activeCallback->streamingHasStopped();
                return;
            }
        }
/*
        if (txStream != nullptr)
        {
            UHDr::StreamCmd streamCmd;
            streamCmd.numSamples = static_cast<size_t> (maxBlockSize);
            streamCmd.streamMode = UHDr::StreamMode::startContinuous;
            streamCmd.streamNow = false;
            streamCmd.timeSpecFracSecs = 0;
            streamCmd.timeSpecFullSecs = delayInSecondsUntilStreamingStarts;

            txStream->issueStreamCmd (streamCmd);
        }
*/
        while (!threadShouldExit())
        {
            if (rxEnabled)
            {
                UHDr::Error error;
                int numSamplesReceived = rxStream->receive (rxBuffer->getArrayOfWritePointers(), maxBlockSize, error, false, 0.5);

                if (error)
                {
                    activeCallback->handleError ("Error executing UHDr::USRP::RxStream::receive: " + UHDr::errorDescription (error) + ". Stopping stream.");
                    activeCallback->streamingHasStopped();
                    return;
                }

                rxBuffer->setNumSamples (numSamplesReceived);
                if (txEnabled)
                    txBuffer->setNumSamples (numSamplesReceived);
            }
            else
            {
                rxBuffer->setNumSamples (0);
                txBuffer->setNumSamples (maxBlockSize);
            }

            // if this value changes in the callback, still process tx data after this callback
            bool txWasEnabled = txEnabled;

            activeCallback->processRFSampleBlock (*rxBuffer, *txBuffer);

            if (txWasEnabled)
            {
                UHDr::Error error;
                txStream->send (txBuffer->getArrayOfWritePointers(), txBuffer->getNumSamples(), error, 0.5);

                if (error)
                {
                    activeCallback->handleError ("Error executing UHDr::USRP::'TxStream::send: " + UHDr::errorDescription (error) + ". Stopping stream.");
                    activeCallback->streamingHasStopped();
                    return;
                }
            }
        }

        activeCallback->streamingHasStopped();
    }

    juce::IPAddress UHDEngine::getIPAddressForMboard (int mboardIdx)
    {
        //todo
        /**
        jassert (juce::isPositiveAndBelow (mboardIdx, numMboardsInUSRP));
        juce::IPAddress ipAddress (mboardProperties[mboardIdx].getProperty (propertyIPAddress, "0.0.0.0").toString());
        // this assert means that there was no property named ip-addr - check the device tree!
        jassert (!ipAddress.isNull());
        return ipAddress;
         */
        return juce::IPAddress ("0.0.0.0");
    }

    juce::String UHDEngineManager::getEngineName () {return "UHD Engine"; }

    juce::Result UHDEngineManager::isEngineAvailable ()
    {
        if (uhdr != nullptr)
            return juce::Result::ok();

#if JUCE_MAC
        juce::String uhdLibName = "libuhd.dylib";
#endif
        juce::DynamicLibrary uhdLib;
        if (uhdLib.open (uhdLibName))
        {
            uhdLib.close();

            juce::String error;
            uhdr = UHDr::load (uhdLibName, error);

            if (uhdr == nullptr)
                return juce::Result::fail (error);

            return juce::Result::ok();
        }

        return juce::Result::fail (uhdLibName + " cannot be found on this system");
    }

    SDRIOEngine* UHDEngineManager::createEngine ()
    {
        return new UHDEngine (uhdr);
    }
}