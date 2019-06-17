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

#ifndef NTLAB_DEBUGPRINT_UHDTREE
#define NTLAB_DEBUGPRINT_UHDTREE 0
#endif

#if JUCE_MAC
#import <Foundation/Foundation.h>
#endif

#include "UHDEngine.h"
#include "../../ErrorHandling/ErrorHandlingMacros.h"

#include <typeinfo> // for std::bad_cast

#if !JUCE_IOS
template <>
struct juce::VariantConverter<ntlab::UHDEngine::SynchronizationSetup>
{
    static ntlab::UHDEngine::SynchronizationSetup fromVar (const juce::var& v)                              { return static_cast<ntlab::UHDEngine::SynchronizationSetup> (static_cast<int> (v)); }
    static juce::var                              toVar   (const ntlab::UHDEngine::SynchronizationSetup& s) { return s; }
};
#endif

namespace ntlab
{
#if !JUCE_IOS
    using varSyncSetupConverter = juce::VariantConverter<UHDEngine::SynchronizationSetup>;

// ugly solution but makes the NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR macro work
#define errorDescription UHDr::errorDescription

    const juce::String UHDEngine::defaultCpuFormat ("fc32");
    const juce::String UHDEngine::defaultOtwFormat ("sc16");
    const juce::String UHDEngine::defaultArgs;
#endif

    const juce::Identifier UHDEngine::propertyUSRPDevice       ("USRP_Device");
    const juce::Identifier UHDEngine::propertyUSRPDeviceConfig ("USRP_Device_config");
    const juce::Identifier UHDEngine::propertyMBoard           ("Mboard");
    const juce::Identifier UHDEngine::propertyMBoards          ("Mboards");
    const juce::Identifier UHDEngine::propertyTimeSources      ("Time_sources");
    const juce::Identifier UHDEngine::propertyClockSources     ("Clock_sources");
    const juce::Identifier UHDEngine::propertySensors          ("Sensors");
    const juce::Identifier UHDEngine::propertyRxDSP            ("RX_DSP");
    const juce::Identifier UHDEngine::propertyTxDSP            ("TX_DSP");
    const juce::Identifier UHDEngine::propertyRxDboard         ("RX_Dboard");
    const juce::Identifier UHDEngine::propertyTxDboard         ("TX_Dboard");
    const juce::Identifier UHDEngine::propertyRxFrontend       ("RX_Frontend");
    const juce::Identifier UHDEngine::propertyTxFrontend       ("TX_Frontend");
    const juce::Identifier UHDEngine::propertyRxCodec          ("RX_Codec");
    const juce::Identifier UHDEngine::propertyTxCodec          ("TX_Codec");
    const juce::Identifier UHDEngine::propertyIPAddress        ("ip-addr");
    const juce::Identifier UHDEngine::propertyID               ("ID");
    const juce::Identifier UHDEngine::propertyName             ("Name");
    const juce::Identifier UHDEngine::propertySerial           ("Serial");
    const juce::Identifier UHDEngine::propertyMin              ("min");
    const juce::Identifier UHDEngine::propertyMax              ("max");
    const juce::Identifier UHDEngine::propertyStepWidth        ("step_width");
    const juce::Identifier UHDEngine::propertyUnit             ("unit");
    const juce::Identifier UHDEngine::propertyUnitScaling      ("unit_scaling");
    const juce::Identifier UHDEngine::propertyCurrentValue     ("current_value");
    const juce::Identifier UHDEngine::propertyArray            ("array");
    const juce::Identifier UHDEngine::propertyFreqRange        ("Freq_range");
    const juce::Identifier UHDEngine::propertyBandwidthRange   ("Bandwidth_range");
    const juce::Identifier UHDEngine::propertyAntennas         ("Antennas");
    const juce::Identifier UHDEngine::propertySyncSetup        ("Synchronization_setup");
    const juce::Identifier UHDEngine::propertySampleRate       ("Sample_rate");

#if !JUCE_IOS

    const juce::Identifier UHDEngine::ChannelMapping::propertyNumChannels      ("num_channels");
    const juce::Identifier UHDEngine::ChannelMapping::propertyHardwareChannel  ("hardware_channel");
    const juce::Identifier UHDEngine::ChannelMapping::propertyMboardIdx        ("mboard_idx");
    const juce::Identifier UHDEngine::ChannelMapping::propertyDboardSlot       ("dboard_slot");
    const juce::Identifier UHDEngine::ChannelMapping::propertyFrontendOnDboard ("frontend_on_dboard");
    const juce::Identifier UHDEngine::ChannelMapping::propertyAntennaPort      ("antenna_port");
    const juce::Identifier UHDEngine::ChannelMapping::propertyAnalogGain       ("analog_gain");
    const juce::Identifier UHDEngine::ChannelMapping::propertyDigitalGain      ("digital_gain");
    const juce::Identifier UHDEngine::ChannelMapping::propertyDigitalGainFine  ("digital_gain_fine");
    const juce::Identifier UHDEngine::ChannelMapping::propertyCenterFrequency  ("center_frequency");
    const juce::Identifier UHDEngine::ChannelMapping::propertyAnalogBandwitdh  ("analog_bandwidth");

    UHDEngine::~UHDEngine ()
    {
        if (isStreaming ())
            stopStreaming ();

        if (threadShouldExit ())
        {
            waitForThreadToExit (2100);
        }
        else
        {
            stopThread (2000);
        }

        std::clog.rdbuf (previousClogStreambuf);
    }

    UHDEngine::UHDEngine (ntlab::UHDr::Ptr uhdLibrary)
      : juce::Thread ("UHD Engine Thread"),
        uhdLogStreambuffer (activeCallback),
        uhdr (uhdLibrary),
        devicesInActiveUSRPSetup ("Active_Devices")
    {
        previousClogStreambuf = std::clog.rdbuf (&uhdLogStreambuffer);
    }

    juce::Result UHDEngine::makeUSRP (juce::StringPairArray& args, SynchronizationSetup synchronizationSetup)
    {
        // rearranging the whole setup while streaming is running seems like a bad idea...
        jassert (!isStreaming());

        // update the device tree if neccesarry
        if (!deviceTree.isValid())
            getDeviceTree();

        UHDr::Error error;
        auto newUSRP = uhdr->makeUSRP (args, error);

        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        // some cleanup
        usrp = nullptr;
        usrp = newUSRP;
        rxStream = nullptr;
        txStream = nullptr;
        rxChannelMapping = nullptr;
        txChannelMapping = nullptr;
        devicesInActiveUSRPSetup.removeAllChildren (nullptr);
        devicesInActiveUSRPSetup.removeAllProperties (nullptr);

        // if all went well we match the device tree with the arg vector to build a tree containing the selected devices
        const int numArgs = args.size();
        auto keys   = args.getAllKeys();
        auto values = args.getAllValues();
        for (int i = 0; i < numArgs; ++i)
        {
            if (keys[i].contains ("addr"))
            {
                for (auto device : deviceTree)
                {
                    if (device.getProperty (propertyIPAddress).toString() == values[i])
                    {
                        // found a device in the tree with matching ip adress. Let's add it to the tree of
                        // devices in the active setup
                        devicesInActiveUSRPSetup.addChild (device.createCopy(), i, nullptr);
                    }
                }
            }
            else
            {
                // todo: support non-ip based connections
                jassertfalse;
            }
        }

        numMboardsInUSRP = usrp->getNumMboards();
        int numMboardsInActiveDeviceTree = devicesInActiveUSRPSetup.getNumChildren();
        if (numMboardsInUSRP != numMboardsInActiveDeviceTree)
        {
            usrp = nullptr;
            jassertfalse;
            return juce::Result::fail ("Could not match device tree and usrp setup");
        }

        if ((synchronizationSetup == singleDeviceStandalone) && (numMboardsInUSRP != 1))
        {
            usrp = nullptr;
            return juce::Result::fail ("You cannot use a single device sync setup if you have more than one device in your setup");

        }
        //todo: how to find out if mboards have mimo port??
        if ((synchronizationSetup == twoDevicesMIMOCableMasterSlave) && (numMboardsInUSRP != 2))
        {
            usrp = nullptr;
            return juce::Result::fail ("You cannot use a MIMO setup with more or less than two devices");
        }

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

    const int UHDEngine::getNumRxChannels ()
    {
        if (rxChannelMapping == nullptr)
            return 0;
        return rxChannelMapping->numChannels;
    }

    const int UHDEngine::getNumTxChannels ()
    {
        if (txChannelMapping == nullptr)
            return 0;
        return txChannelMapping->numChannels;
    }

    juce::Result UHDEngine::setupRxChannels (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup)
    {
        if (numMboardsInUSRP == 0)
            return juce::Result::fail ("No motherboards found");

        rxChannelMapping.reset (new ChannelMapping (channelSetup, *this, ChannelMapping::Direction::rx));
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

        if (error)
        {
            rxStream.reset (nullptr);
            return juce::Result::fail ("Error creating Rx Stream: " + usrp->getLastUSRPError ());
        }

        std::vector<juce::StringArray> gainElements;
        for (int c = 0; c < rxChannelMapping->numChannels; ++c)
        {
            gainElements.emplace_back (usrp->getValidRxGainElements (c));
            error = usrp->setRxAntenna (channelSetup[c].antennaPort.toRawUTF8 (), c);
            if (error)
            {
                rxStream.reset (nullptr);
                return juce::Result::fail ("Error setting Rx Antenna: " + usrp->getLastUSRPError ());
            }
        }
        rxChannelMapping->setGainElements (std::move (gainElements));

        rxEnabled = true;

        return juce::Result::ok();
    }

    juce::Result UHDEngine::setupTxChannels (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup)
    {
        if (numMboardsInUSRP == 0)
            return juce::Result::fail ("No motherboards found");

        txChannelMapping.reset (new ChannelMapping (channelSetup, *this, ChannelMapping::Direction::tx));
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

        if (error)
        {
            rxStream.reset (nullptr);
            return juce::Result::fail ("Error creating Tx Stream: " + usrp->getLastUSRPError ());
        }

        std::vector<juce::StringArray> gainElements;
        for (int c = 0; c < txChannelMapping->numChannels; ++c)
        {
            gainElements.emplace_back (usrp->getValidTxGainElements (c));
            error = usrp->setTxAntenna (channelSetup[c].antennaPort.toRawUTF8 (), c);
            if (error)
            {
                rxStream.reset (nullptr);
                return juce::Result::fail ("Error setting Rx Antenna: " + usrp->getLastUSRPError ());
            }
        }
        txChannelMapping->setGainElements (std::move (gainElements));

        txEnabled = true;

        return juce::Result::ok();
    }

    bool UHDEngine::setSampleRate (double newSampleRate)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);

        // you cannot set the sample rate before having set up neither tx nor rx channels
        jassert ((rxChannelMapping != nullptr) || (txChannelMapping != nullptr));

        if (rxChannelMapping != nullptr)
        {
            for (int c = 0; c < rxChannelMapping->numChannels; ++c)
            {
                auto frontend = rxChannelMapping->getFrontendForBufferChannel (c);
                auto bandwidthRange = frontend.getChildWithName (propertyBandwidthRange);
                if (bandwidthRange.isValid())
                {
                    const double scaling    = static_cast<double> (bandwidthRange.getProperty (propertyUnitScaling));
                    double currentBandwidth = static_cast<double> (bandwidthRange.getProperty (propertyCurrentValue));
                    // in this case the current setting is unknown. We just try to catch the worst case that the sample
                    // rate is even higher than what hardware can theoretically handle. However, if it is set to a lower
                    // bandwidth it might go wrong anyway
                    if (std::isnan (currentBandwidth))
                        currentBandwidth = static_cast<double> (bandwidthRange.getProperty (propertyMax));

                    if (newSampleRate > (currentBandwidth * scaling))
                    {
                        // how do you expect a sample rate to work that is greater than the analog bandwidth of your frontend?
                        jassertfalse;
                        return false;
                    }
                }

                auto setSampleRate = usrp->setRxSampleRate (newSampleRate, c);
                NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED (setSampleRate);
            }
        }

        if (txChannelMapping != nullptr)
        {
            for (int c = 0; c < txChannelMapping->numChannels; ++c)
            {
                auto frontend = txChannelMapping->getFrontendForBufferChannel (c);
                auto bandwidthRange = frontend.getChildWithName (propertyBandwidthRange);
                if (bandwidthRange.isValid())
                {
                    const double scaling      = static_cast<double> (bandwidthRange.getProperty (propertyUnitScaling));
                    double currentBandwidth = static_cast<double> (bandwidthRange.getProperty (propertyCurrentValue));
                    // in this case the current setting is unknown. We just try to catch the worst case that the sample
                    // rate is even higher than what hardware can theoretically handle. However, if it is set to a lower
                    // bandwidth it might go wrong anyway
                    if (std::isnan (currentBandwidth))
                        currentBandwidth = static_cast<double> (bandwidthRange.getProperty (propertyMax));

                    if (newSampleRate > (currentBandwidth * scaling))
                    {
                        // how do you expect a sample rate to work that is greater than the analog bandwidth of your frontend?
                        jassertfalse;
                        return false;
                    }
                }

                auto setSampleRate = usrp->setTxSampleRate (newSampleRate, c);
                NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED (setSampleRate);
            }
        }

        return true;
    }

    double UHDEngine::getSampleRate ()
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;

        double rxSampleRate, txSampleRate;
        if (rxChannelMapping != nullptr)
        {
            rxSampleRate = usrp->getRxSampleRate (0, error);
            NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED (error);
        }

        if (txChannelMapping != nullptr)
        {
            txSampleRate = usrp->getTxSampleRate (0, error);
            NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED (error);
        }

        if ((txChannelMapping != nullptr) && (rxChannelMapping != nullptr) && (rxSampleRate != txSampleRate))
        {
            lastError = "Error getting sample rate, different samplerates for rx (" + juce::String (rxSampleRate) + "Hz) and tx (" + juce::String (txSampleRate) + "Hz) returned";
            DBG (lastError);
            return -1.0;
        }

        if (rxChannelMapping != nullptr)
            return rxSampleRate;

        if (txChannelMapping != nullptr)
            return txSampleRate;

        // trying to get the samplerate without having configured neiter an rx nor a tx stream seems like an unintended action...
        jassertfalse;
        return 0.0;
    }

    juce::ValueTree UHDEngine::getDeviceTree()
    {
        deviceTree = getUHDTree();

        numMboardsInDeviceTree = deviceTree.getNumChildren();

        return deviceTree;
    }

    juce::ValueTree UHDEngine::getActiveConfig ()
    {
        juce::ValueTree activeSetup (propertyUSRPDeviceConfig);
        activeSetup.setProperty (propertySyncSetup, synchronizationSetup, nullptr);
        activeSetup.setProperty (propertySampleRate, getSampleRate(), nullptr);

        juce::ValueTree mboardsInSetup (propertyMBoards);
        activeSetup.addChild (mboardsInSetup, -1, nullptr);
        int mboardIdx = 0;

        for (const auto& mboard : devicesInActiveUSRPSetup)
        {
            juce::ValueTree mboardInformation (mboard.getType());
            mboardsInSetup.addChild (mboardInformation, mboardIdx, nullptr);

            mboardInformation.setProperty (propertyMBoard, mboard.getProperty (propertyMBoard), nullptr);
            mboardInformation.setProperty (propertyIPAddress, mboard.getProperty (propertyIPAddress), nullptr);

            ++mboardIdx;
        }

        if (rxChannelMapping != nullptr)
            activeSetup.addChild (rxChannelMapping->serializeCurrentSetup (ChannelMapping::Direction::rx), -1, nullptr);

        if (txChannelMapping != nullptr)
            activeSetup.addChild (txChannelMapping->serializeCurrentSetup (ChannelMapping::Direction::tx), -1, nullptr);

        return activeSetup;
    }

    juce::Result UHDEngine::setConfig (juce::ValueTree& configToSet)
    {
        if (!configToSet.hasType (propertyUSRPDeviceConfig))
            return juce::Result::fail ("Expecting a config of type " + propertyUSRPDeviceConfig.toString () + " but got a config of type " + configToSet.getType ().toString ());

        auto mboards = configToSet.getChildWithName (propertyMBoards);
        auto syncSetup  = configToSet.getProperty (propertySyncSetup);
        auto sampleRate = configToSet.getProperty (propertySampleRate);

        if (!mboards.isValid ())
            return juce::Result::fail ("Invalid config, missing Mboards entries");

        if (syncSetup.isVoid())
            return juce::Result::fail ("Invalid config, missing synchronization setup entry");

        if (sampleRate.isVoid())
            return juce::Result::fail ("Invalid config, missing sample rate entry");

        const int numMboards = mboards.getNumChildren();
        juce::StringPairArray args;

        for (int m = 0; m < numMboards; ++m)
        {
            auto ipAddress = mboards.getChild (m).getProperty (propertyIPAddress);
            jassert (!ipAddress.isVoid());
            args.set ("addr" + juce::String (m), ipAddress.toString ());
        }

        auto makingUSRP = makeUSRP (args, varSyncSetupConverter::fromVar (syncSetup));
        if (makingUSRP.failed())
            return makingUSRP;

        auto rxSetup = configToSet.getChildWithName ("Rx_Channel_Setup");
        auto txSetup = configToSet.getChildWithName ("Tx_Channel_Setup");

        if (rxSetup.isValid ())
        {
            auto deserializingRxSetup = ChannelMapping::deserializeSetup (rxSetup, *this);
            if (deserializingRxSetup.failed ())
                return deserializingRxSetup;
        }

        if (txSetup.isValid ())
        {
            auto deserializingTxSetup = ChannelMapping::deserializeSetup (txSetup, *this);
            if (deserializingTxSetup.failed ())
                return deserializingTxSetup;
        }

        if ((rxChannelMapping != nullptr) || (txChannelMapping != nullptr))
        {
            auto successSettingSampleRate = setSampleRate (sampleRate);
            if (!successSettingSampleRate)
                return juce::Result::fail ("Error setting restoring sample rate " + sampleRate.toString());
        }

        return juce::Result::ok();
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
        stopThread (20000);
    }

    bool UHDEngine::isStreaming ()
    {
        return isThreadRunning();
    }

    bool UHDEngine::enableRxTx (bool enableRx, bool enableTx)
    {
        // You can not disable both, rx and tx streams. Call stopStreaming for this.
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (!(enableRx || enableTx));

        // You cannot enable rx if you haven set up your rx channels successfully
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (enableRx && (rxStream == nullptr));

        // You cannot enable rx if you haven set up your tx channels successfully
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (enableTx && (txStream == nullptr));

        rxEnabled = enableRx;
        txEnabled = enableTx;

        return true;
    }

    bool UHDEngine::setRxCenterFrequency (double newCenterFrequency, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (rxChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < rxChannelMapping->numChannels; ++c)
            {
                NTLAB_RETURN_FALSE_IF (setRxCenterFrequency (newCenterFrequency, c) == false);
            }
        }
        else
        {
            UHDr::TuneResult  result;
            UHDr::TuneRequest request;
            char requestArgs[] = "";
            request.targetFreq = newCenterFrequency;
            request.args = requestArgs;

            auto setRxFrequency  = rxChannelMapping->isFrontendPropertyInValidRange (channel, propertyFreqRange, newCenterFrequency);
            auto hardwareChannel = rxChannelMapping->getHardwareChannelForBufferChannel (channel);

            if (setRxFrequency.wasOk())
            {
                auto error = usrp->setRxFrequency (request, result, hardwareChannel);
                if (error)
                    setRxFrequency = juce::Result::fail (errorDescription (error));
            }

            if (setRxFrequency.failed())
            {
                DBG (lastError = setRxFrequency.getErrorMessage());
                return false;
            }

            notifyListenersRxCenterFreqChanged (result.actualRfFreq, channel);

            //if (!((result.actualRfFreq == result.targetRfFreq) && (result.actualDspFreq == result.targetDspFreq)))
            if (!juce::approximatelyEqual (result.actualRfFreq, result.targetRfFreq))
            {
                DBG (lastError = "Error setting exact Rx center frequency. Target Rf frequency: "
                        + juce::String (result.targetRfFreq)  + "Hz, actual Rf frequency: "
                        + juce::String (result.actualRfFreq)  + "Hz, target DSP frequency: "
                        + juce::String (result.targetDspFreq) + "Hz, actual DSP frequency: "
                        + juce::String (result.actualDspFreq) + "Hz");
                return false;
            }
        }
        return true;
    }

    bool UHDEngine::setTxCenterFrequency (double newCenterFrequency, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (txChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < txChannelMapping->numChannels; ++c)
            {
                NTLAB_RETURN_FALSE_IF (setTxCenterFrequency (newCenterFrequency, c) == false);
            }
        }
        else
        {
            UHDr::TuneResult result;
            UHDr::TuneRequest request;
            char requestArgs[] = "";
            request.targetFreq = newCenterFrequency;
            request.args = requestArgs;

            auto setTxFrequency  = txChannelMapping->isFrontendPropertyInValidRange (channel, propertyFreqRange, newCenterFrequency);
            auto hardwareChannel = txChannelMapping->getHardwareChannelForBufferChannel (channel);

            if (setTxFrequency.wasOk())
            {
                auto error = usrp->setTxFrequency (request, result, hardwareChannel);
                if (error)
                    setTxFrequency = juce::Result::fail (errorDescription (error));
            }

            if (setTxFrequency.failed())
            {
                DBG (lastError = setTxFrequency.getErrorMessage());
                return false;
            }

            notifyListenersTxCenterFreqChanged (result.actualRfFreq, channel);

            //if (!((result.actualRfFreq == result.targetRfFreq) && (result.actualDspFreq == result.targetDspFreq)))
            if (!juce::approximatelyEqual (result.actualRfFreq, result.targetRfFreq))
            {
                DBG (lastError = "Error setting exact Tx center frequency. Target Rf frequency: "
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
        double rxFrequency = usrp->getRxFrequency (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, rxFrequency);
    }

    double UHDEngine::getTxCenterFrequency (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double txFrequency = usrp->getTxFrequency (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txFrequency);
    }

    bool UHDEngine::setRxBandwidth (double newBandwidth, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (rxChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < rxChannelMapping->numChannels; ++c)
            {
                NTLAB_RETURN_FALSE_IF (setRxBandwidth (newBandwidth, c) == false);
            }
        }
        else
        {
            auto setRxBandwidth  = rxChannelMapping->isFrontendPropertyInValidRange (channel, propertyBandwidthRange, newBandwidth);
            auto hardwareChannel = rxChannelMapping->getHardwareChannelForBufferChannel (channel);

            if (setRxBandwidth.wasOk())
            {
                auto error = usrp->setRxBandwidth (newBandwidth, hardwareChannel);
                if (error)
                    setRxBandwidth = juce::Result::fail (errorDescription (error));
            }

            if (setRxBandwidth.wasOk())
                notifyListenersRxBandwidthChanged (newBandwidth, channel);

            NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setRxBandwidth);
        }
        return true;
    }


    bool UHDEngine::setTxBandwidth (double newBandwidth, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (txChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < txChannelMapping->numChannels; ++c)
            {
                NTLAB_RETURN_FALSE_IF (setTxBandwidth (newBandwidth, c) == false);
            }
        }
        else
        {
            auto setTxBandwidth  = txChannelMapping->isFrontendPropertyInValidRange (channel, propertyBandwidthRange, newBandwidth);
            auto hardwareChannel = txChannelMapping->getHardwareChannelForBufferChannel (channel);

            if (setTxBandwidth.wasOk())
            {
                auto error = usrp->setTxBandwidth (newBandwidth, hardwareChannel);
                if (error)
                    setTxBandwidth = juce::Result::fail (errorDescription (error));
            }

            if (setTxBandwidth.wasOk())
                notifyListenersTxBandwidthChanged (newBandwidth, channel);

            NTLAB_RETURN_FALSE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_TRUE_OTHERWISE (setTxBandwidth);
        }
        return true;
    }

    double UHDEngine::getRxBandwidth (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double rxBandwidth = usrp->getRxBandwidth (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, rxBandwidth);
    }

    double UHDEngine::getTxBandwidth (int channel)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        UHDr::Error error;
        double txBandwidth = usrp->getTxBandwidth (channel, error);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txBandwidth);
    }

    bool UHDEngine::setRxGain (double newGain, ntlab::SDRIOHardwareEngine::GainElement gainElement, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (rxChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < rxChannelMapping->numChannels; ++c)
            {
                NTLAB_RETURN_FALSE_IF (setRxGain (newGain, gainElement, c) == false);
            }
            return true;
        }

        char* gainElementString = nullptr;
        switch (gainElement)
        {
            case unspecified:
                gainElementString = const_cast<char*> (rxChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::automatic, newGain));
                break;
            case analog:
                gainElementString = const_cast<char*> (rxChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::analog, newGain));
                break;
            case digital:
            {
                double requiredCoarseGain, requiredFineGain;
                rxChannelMapping->digitalGainPartition (channel, newGain, requiredCoarseGain, requiredFineGain);
                auto coarseGainElementString = rxChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::digital, requiredCoarseGain);
                if (coarseGainElementString == nullptr)
                {
                    // invalid digital gain value
                    jassertfalse;
                    return false;
                }
                if (*coarseGainElementString == 0)
                {
                    // device seems to have no digital gain element
                    jassertfalse;
                    return false;
                }
                auto error = usrp->setRxGain (requiredCoarseGain, rxChannelMapping->getHardwareChannelForBufferChannel (channel), coarseGainElementString);
                jassert (error == UHDr::Error::errorNone);
                if (error)
                    return false;

                auto fineGainElementString = rxChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::digitalFine, requiredFineGain);
                if ((fineGainElementString != nullptr) && (*fineGainElementString != 0))
                {
                    error = usrp->setRxGain (requiredFineGain, rxChannelMapping->getHardwareChannelForBufferChannel (channel), fineGainElementString);
                    jassert (error == UHDr::Error::errorNone);
                    if (error)
                        return false;
                }
                return true;
            }
        }
        if (gainElementString == nullptr)
        {
            // invalid gain value
            jassertfalse;
            return false;
        }
        auto error = usrp->setRxGain (newGain, rxChannelMapping->getHardwareChannelForBufferChannel (channel), gainElementString);
        jassert (error == UHDr::Error::errorNone);
        return error ? false : true;
    }

    bool UHDEngine::setTxGain (double newGain, ntlab::SDRIOHardwareEngine::GainElement gainElement, int channel)
    {
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (usrp == nullptr);
        NTLAB_RETURN_FALSE_AND_ASSERT_IF (txChannelMapping == nullptr);

        if (channel == allChannels)
        {
            for (int c = 0; c < txChannelMapping->numChannels; ++c)
            {
                if (setTxGain (newGain, gainElement, c) == false)
                    return false;
            }
            return true;
        }

        char* gainElementString = nullptr;
        switch (gainElement)
        {
            case unspecified:
                gainElementString = const_cast<char*> (txChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::automatic, newGain));
                break;
            case analog:
                gainElementString = const_cast<char*> (txChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::analog, newGain));
                break;
            case digital:
            {
                double requiredCoarseGain, requiredFineGain;
                rxChannelMapping->digitalGainPartition (channel, newGain, requiredCoarseGain, requiredFineGain);
                auto coarseGainElementString = txChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::digital, requiredCoarseGain);
                if (coarseGainElementString == nullptr)
                {
                    // invalid digital gain value
                    jassertfalse;
                    return false;
                }
                if (*coarseGainElementString == 0)
                {
                    // device seems to have no digital gain element
                    jassertfalse;
                    return false;
                }
                auto error = usrp->setTxGain (requiredCoarseGain, txChannelMapping->getHardwareChannelForBufferChannel (channel), coarseGainElementString);
                jassert (error == UHDr::Error::errorNone);
                if (error)
                    return false;

                auto fineGainElementString = txChannelMapping->getGainElementStringIfGainIsInValidRange (channel, ChannelMapping::UHDGainElements::digitalFine, requiredFineGain);
                if ((fineGainElementString != nullptr) && (*fineGainElementString != 0))
                {
                    error = usrp->setTxGain (requiredFineGain, txChannelMapping->getHardwareChannelForBufferChannel (channel), fineGainElementString);
                    jassert (error == UHDr::Error::errorNone);
                    if (error)
                        return false;
                }
                return true;
            }
        }
        if (gainElementString == nullptr)
        {
            // invalid gain value
            jassertfalse;
            return false;
        }
        auto error = usrp->setTxGain (newGain, txChannelMapping->getHardwareChannelForBufferChannel (channel), gainElementString);
        jassert (error == UHDr::Error::errorNone);
        return error ? false : true;
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

    double UHDEngine::getTxGain (int channel, ntlab::SDRIOHardwareEngine::GainElement gainElement)
    {
        NTLAB_RETURN_MINUS_ONE_IF (usrp == nullptr);

        // todo: currently the gain element is not mapped to any UHD compatible gain element string. Find out if there is a proper mapping
        const char* gainElementString = "";
        UHDr::Error error;
        double txGain = usrp->getTxGain (channel, error, gainElementString);

        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, txGain);
    }

    UHDEngine::UHDLogStreambuffer::UHDLogStreambuffer (SDRIODeviceCallback*& callbackToUse) : callback (callbackToUse)
    {
        setp (nullptr, nullptr);
    }

    std::streamsize UHDEngine::UHDLogStreambuffer::xsputn (const char* string, std::streamsize count)
    {
        logTempBuffer += juce::String::fromUTF8 (string, static_cast<int> (count));
        return count;
    }

    std::streambuf::int_type UHDEngine::UHDLogStreambuffer::overflow (std::streambuf::int_type character)
    {
        if (callback != nullptr)
        {
            if (logTempBuffer.contains ("[ERROR]"))
            {
                callback->handleError (logTempBuffer);
            }
        }
        else
        {
#if JUCE_MAC
            CFStringRef messageCFString = logTempBuffer.toCFString();
            NSLog (@"%@", (NSString*)messageCFString);
            CFRelease (messageCFString);
#else
            std::cerr << logTempBuffer;
#endif
        }

        logTempBuffer.clear();
        return character;
    }

    UHDEngine::ChannelMapping::ChannelMapping (juce::Array<ntlab::UHDEngine::ChannelSetup>& channelSetup, UHDEngine& engine, ChannelMapping::Direction direction)
      : numChannels (channelSetup.size()),
        channelSetupHardwareOrder (channelSetup),
        gainElementSubtree (static_cast<size_t> (numChannels)),
        gainElementsMap    (static_cast<size_t> (numChannels)),
        uhdEngine (engine)
    {
        int numChannels = channelSetup.size();
        size_t hardwareChannel = 0;
        bufferOrderToHardwareOrder.resize (numChannels);

        juce::Identifier propertyDboard;
        juce::Identifier propertyFrontend;
        juce::Identifier propertyCodec;
        if (direction == rx)
        {
            propertyDboard   = propertyRxDboard;
            propertyFrontend = propertyRxFrontend;
            propertyCodec    = propertyRxCodec;
        }
        else
        {
            propertyDboard   = propertyTxDboard;
            propertyFrontend = propertyTxFrontend;
            propertyCodec    = propertyTxCodec;
        }

        for (int m = 0; m < engine.numMboardsInUSRP; ++m)
        {
            juce::String motherboardSubdevSpec;

            for (int c = 0; c < numChannels; ++c)
            {
                // if this assert is hit one of the mboardIdx values was out of range
                jassert (juce::isPositiveAndBelow (channelSetup[c].mboardIdx, engine.numMboardsInUSRP));
                if (channelSetup[c].mboardIdx == m)
                {
                    auto mboard = engine.devicesInActiveUSRPSetup.getChild (m);
                    auto dboards = mboard.getChildWithName (propertyDboard);

                    bool foundDboardInActiveSetup = false, foundFrontendInActiveSetup = false;
                    for (auto dboard : dboards)
                    {
                        if (dboard.hasType (channelSetup[c].daughterboardSlot))
                        {
                            foundDboardInActiveSetup = true;
                            auto frontends = dboard.getChildWithName (propertyFrontend);
                            for (auto frontend : frontends)
                            {
                                auto frontendType = frontend.getType().toString().replaceCharacter ('_', ' ').trimStart();
                                if (frontendType == channelSetup[c].frontendOnDaughterboard)
                                {
                                    // expecting all devices to use codec A. If you hit an assert here, please open an
                                    // issue on GitHub
                                    static const juce::Identifier propertyA ("A");
                                    auto codec = dboard.getChildWithName (propertyCodec).getChildWithName (propertyA);
                                    jassert (codec.isValid());

                                    foundFrontendInActiveSetup = true;
                                    motherboardSubdevSpec += channelSetup[c].daughterboardSlot + ":" + channelSetup[c].frontendOnDaughterboard + " ";
                                    auto antennas = frontend.getChildWithName (propertyAntennas);
                                    validAntennas.emplace_back (juce::StringArray::fromTokens (antennas.getProperty (propertyArray).toString(), ",", ""));
                                    mboardBufferOrder  .add (mboard);
                                    dboardBufferOrder  .add (dboard);
                                    frontendBufferOrder.add (frontend);
                                    codecBufferOrder   .add (codec);
                                    bufferOrderToHardwareOrder.set (c, hardwareChannel);
                                    ++hardwareChannel;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    // you passed an invalid daughterboard or frontend slot
                    jassert (foundDboardInActiveSetup && foundFrontendInActiveSetup);
                }
            }

            motherboardSubdevSpec.trimEnd();
            subdevSpecs.add (motherboardSubdevSpec);
        }

        for (auto& map : gainElementsMap)
        {
            map.fill (-1);
            map[automatic] = -2;
        }

    }

    void UHDEngine::ChannelMapping::setGainElements (std::vector<juce::StringArray>&& newGainElements)
    {
        jassert (numChannels == newGainElements.size());
        gainElements = newGainElements;

        for (int c = 0; c < numChannels; ++c)
        {
            gainElementSubtree[c].resize (UHDGainElements::count);

            int idxOfAnalogGain, idxOfDigitalGainCoarse, idxOfDigitalGainFine;
            if ((idxOfAnalogGain = gainElements[c].indexOf ("PGA0")) > -1)
            {
                gainElementsMap[c][UHDGainElements::analog] = idxOfAnalogGain;
                static const juce::Identifier propertyGainRangePGA0 ("Gain_range_PGA0");
                gainElementSubtree[c][UHDGainElements::analog] = frontendBufferOrder[c].getChildWithName (propertyGainRangePGA0);
            }
            if ((idxOfDigitalGainCoarse = gainElements[c].indexOf ("ADC-digital")) > -1)
            {
                gainElementsMap[c][UHDGainElements::digital] = idxOfDigitalGainCoarse;
                static const juce::Identifier propertyGainRangeDigital ("Gain_range_digital");
                gainElementSubtree[c][UHDGainElements::digital] = codecBufferOrder[c].getChildWithName (propertyGainRangeDigital);
            }
            if ((idxOfDigitalGainFine = gainElements[c].indexOf ("ADC-fine")) > -1)
            {
                gainElementsMap[c][UHDGainElements::digitalFine] = idxOfDigitalGainFine;
                static const juce::Identifier propertyGainRangeFine ("Gain_range_fine");
                gainElementSubtree[c][UHDGainElements::digitalFine] = codecBufferOrder[c].getChildWithName (propertyGainRangeFine);
            }

#if NTLAB_WARN_ABOUT_MISSING_UHD_GAIN_ELEMENTS
            // warn if no match could be found for one of the three gain elements. The most likely source of this error
            // is that the name of the gain element for the hardware you use does not match the checks above. In this case
            // a suitable check case should be added, feel free to create a pull request for such an addition. In all
            // other cases you can suppress this warning by disabling NTLAB_WARN_ABOUT_MISSING_UHD_GAIN_ELEMENTS
            for (UHDGainElements g = analog; g < UHDGainElements::count; ++g)
            {
                if (gainElementsMap[c][g] == -1)
                {
                    std::cerr << "Warning in " << BOOST_CURRENT_FUNCTION << ": Could not find a matching " << g <<
                                 " gain element for frontend " << dboardBufferOrder[c].getProperty (propertyID).toString() <<
                                 ", Serial " << dboardBufferOrder[c].getProperty (propertySerial).toString() <<
                                 ".\nPossible gain element names for this dboard are: " << gainElements[c].joinIntoString (", ") <<
                                 ".\nIf one of these names seems to refer to the " << g << " gain element, add a check"
                                 " for this name to the function." << std::endl;
                }
            }
#endif
        }
    }

    int UHDEngine::ChannelMapping::getHardwareChannelForBufferChannel (int bufferChannel) const {return static_cast<int> (bufferOrderToHardwareOrder[bufferChannel]); }

    int UHDEngine::ChannelMapping::getMboardIdxForBufferChannel (int bufferChannel) const {return channelSetupHardwareOrder[static_cast<int> (bufferOrderToHardwareOrder[bufferChannel])].mboardIdx; }

    const juce::ValueTree UHDEngine::ChannelMapping::getMboardForBufferChannel (int bufferChannel) const {return mboardBufferOrder[bufferChannel]; }

    const juce::ValueTree UHDEngine::ChannelMapping::getDboardForBufferChannel (int bufferChannel) const {return dboardBufferOrder[bufferChannel]; }

    const juce::ValueTree UHDEngine::ChannelMapping::getFrontendForBufferChannel (int bufferChannel) const {return frontendBufferOrder[bufferChannel]; }

    const juce::StringArray& UHDEngine::ChannelMapping::getValidAntennas (int bufferChannel) const {return validAntennas[bufferChannel]; }

    bool UHDEngine::ChannelMapping::isValidAntennaForChannel (const char* antennaName, int bufferChannel) const
    {
        juce::StringRef nameToCheck (antennaName);
        return validAntennas[bufferChannel].contains (nameToCheck);
    }

    juce::Result UHDEngine::ChannelMapping::isFrontendPropertyInValidRange (const int channel, const juce::Identifier& propertyToCheck, double valueToTest, bool updateTreeIfValid)
    {
        auto propertySubtree = frontendBufferOrder[channel].getChildWithName (propertyToCheck);

        const double scaling = propertySubtree.getProperty (propertyUnitScaling);
        jassert (scaling != 0); // the property seems to have no scaling value but should have. Maybe no range?
        const double rangeMin = static_cast<double> (propertySubtree.getProperty (propertyMin));
        const double rangeMax = static_cast<double> (propertySubtree.getProperty (propertyMax));

        valueToTest /= scaling;

        if ((valueToTest >= rangeMin) && (valueToTest <= rangeMax))
        {
            if (updateTreeIfValid)
                propertySubtree.setProperty (propertyCurrentValue, valueToTest, nullptr);

            return juce::Result::ok();
        }

        const juce::String unit = propertySubtree.getProperty (propertyUnit);

        if (rangeMin == rangeMax)
            return juce::Result::fail ("Error setting " + propertyToCheck.toString() + " for channel " +
                                       juce::String (channel)  + ": The value is fixed to " +
                                       juce::String (rangeMin) + unit + " and cannot be adjusted for this hardware device.");

        return juce::Result::fail ("Error setting " + propertyToCheck.toString() + " for channel " +
                                   juce::String (channel) + ": Desired value " +
                                   juce::String (valueToTest) + unit + " is out of valid range (" +
                                   juce::String (rangeMin)    + unit + " to " +
                                   juce::String (rangeMax)    + unit + ")");
    }

    const juce::StringArray& UHDEngine::ChannelMapping::getSubdevSpecs() const {return subdevSpecs; }

    const char* UHDEngine::ChannelMapping::getGainElementStringIfGainIsInValidRange (const int bufferChannel, ntlab::UHDEngine::ChannelMapping::UHDGainElements uhdGainElement, double gainValueToCheck)
    {
        int gainElementIdx = gainElementsMap[bufferChannel][uhdGainElement];
        if (gainElementIdx < 0)
        {
            // no such gain element. Compute the overall possible gain instead
            double overallMaxGain = 0.0;
            double overallMinGain = 0.0;

            for (int gainElementIdx = 0; gainElementIdx < UHDGainElements::count; ++gainElementIdx)
            {
                if (gainElementsMap[bufferChannel][gainElementIdx] >= 0)
                {
                    // the element exists
                    const double scaling = static_cast<double> (gainElementSubtree[bufferChannel][gainElementIdx].getProperty (propertyUnitScaling));
                    overallMinGain      += static_cast<double> (gainElementSubtree[bufferChannel][gainElementIdx].getProperty (propertyMin)) * scaling;
                    overallMaxGain      += static_cast<double> (gainElementSubtree[bufferChannel][gainElementIdx].getProperty (propertyMax)) * scaling;
                }
            }

            jassert (overallMinGain <= overallMaxGain);

            if ((overallMinGain <= gainValueToCheck) && (overallMaxGain >= gainValueToCheck))
                return emptyGainElementString;
        }
        else
        {
            const double scaling = static_cast<double> (gainElementSubtree[bufferChannel][uhdGainElement].getProperty (propertyUnitScaling));
            const double minGain = static_cast<double> (gainElementSubtree[bufferChannel][uhdGainElement].getProperty (propertyMin)) * scaling;
            const double maxGain = static_cast<double> (gainElementSubtree[bufferChannel][uhdGainElement].getProperty (propertyMax)) * scaling;

            if ((minGain <= gainValueToCheck) && (maxGain >= gainValueToCheck))
                return gainElements[bufferChannel][gainElementIdx].toRawUTF8();
        }
        return nullptr;
    }

    void UHDEngine::ChannelMapping::digitalGainPartition (const int bufferChannel, const double desiredGain, double& requiredCoarseGain, double& requiredFineGain)
    {
        if (!gainElementSubtree[bufferChannel][UHDGainElements::digitalFine].isValid())
        {
            requiredCoarseGain = desiredGain;
            requiredFineGain = 0.0;
            return;
        }
        const double coarseScaling   = static_cast<double> (gainElementSubtree[bufferChannel][UHDGainElements::digital].getProperty (propertyUnitScaling));
        const double coarseStepWidth = static_cast<double> (gainElementSubtree[bufferChannel][UHDGainElements::digital].getProperty (propertyStepWidth)) * coarseScaling;

        const double ratioCoarse = desiredGain / coarseStepWidth;
        const double nStepsCoarse = std::floor (ratioCoarse);
        requiredCoarseGain = nStepsCoarse * coarseStepWidth;

        if (ratioCoarse == nStepsCoarse)
        {
            requiredFineGain = 0;
            return;
        }

        requiredFineGain = (ratioCoarse - nStepsCoarse) * coarseStepWidth;
    }

    size_t* UHDEngine::ChannelMapping::getStreamArgsChannelList() {return bufferOrderToHardwareOrder.data(); }

    juce::ValueTree UHDEngine::ChannelMapping::serializeCurrentSetup (ntlab::UHDEngine::ChannelMapping::Direction direction)
    {
        juce::Identifier setupType = juce::String::formatted ("%cx_Channel_Setup", direction);

        juce::ValueTree currentSetup (setupType);
        currentSetup.setProperty (propertyNumChannels, numChannels, nullptr);

        for (int c = 0; c < numChannels; ++c)
        {
            juce::Identifier propertyChannelC = "Channel_" + juce::String (c);
            juce::ValueTree channelTree (propertyChannelC);

            currentSetup.addChild (channelTree, c, nullptr);

            const int hardwareChannel = static_cast<int> (bufferOrderToHardwareOrder[c]);
            const auto& channelSetup = channelSetupHardwareOrder[hardwareChannel];

            channelTree.setProperty (propertyMboardIdx,        channelSetup.mboardIdx,               nullptr);
            channelTree.setProperty (propertyDboardSlot,       channelSetup.daughterboardSlot,       nullptr);
            channelTree.setProperty (propertyFrontendOnDboard, channelSetup.frontendOnDaughterboard, nullptr);
            channelTree.setProperty (propertyAntennaPort,      channelSetup.antennaPort,             nullptr);

            const int idxOfAnalogGainElement      = gainElementsMap[c][UHDGainElements::analog];
            const int idxOfDigitalGainElement     = gainElementsMap[c][UHDGainElements::digital];
            const int idxOfDigitalFineGainElement = gainElementsMap[c][UHDGainElements::digitalFine];

            UHDr::Error error;

            if (direction == rx)
            {
                if (idxOfAnalogGainElement >= 0)
                {
                    channelTree.setProperty (propertyAnalogGain, uhdEngine.usrp->getRxGain (hardwareChannel, error, gainElements[c][idxOfAnalogGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyAnalogGain, nullptr);
                }

                if (idxOfDigitalGainElement >= 0)
                {
                    channelTree.setProperty (propertyDigitalGain, uhdEngine.usrp->getRxGain (hardwareChannel, error, gainElements[c][idxOfDigitalGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyDigitalGain, nullptr);
                }

                if (idxOfDigitalFineGainElement >= 0)
                {
                    channelTree.setProperty (propertyDigitalGainFine, uhdEngine.usrp->getRxGain (hardwareChannel, error, gainElements[c][idxOfDigitalFineGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyDigitalGainFine, nullptr);
                }

                channelTree.setProperty (propertyCenterFrequency, uhdEngine.getRxCenterFrequency (c), nullptr);
                channelTree.setProperty (propertyAnalogBandwitdh, uhdEngine.getRxBandwidth       (c), nullptr);
            }
            else
            {
                if (idxOfAnalogGainElement >= 0)
                {
                    channelTree.setProperty (propertyAnalogGain, uhdEngine.usrp->getTxGain (hardwareChannel, error, gainElements[c][idxOfAnalogGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyAnalogGain, nullptr);
                }

                if (idxOfDigitalGainElement >= 0)
                {
                    channelTree.setProperty (propertyDigitalGain, uhdEngine.usrp->getTxGain (hardwareChannel, error, gainElements[c][idxOfDigitalGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyDigitalGain, nullptr);
                }

                if (idxOfDigitalFineGainElement >= 0)
                {
                    channelTree.setProperty (propertyDigitalGainFine, uhdEngine.usrp->getTxGain (hardwareChannel, error, gainElements[c][idxOfDigitalFineGainElement].toRawUTF8()), nullptr);
                    if (error) channelTree.removeProperty (propertyDigitalGainFine, nullptr);
                }

                channelTree.setProperty (propertyCenterFrequency, uhdEngine.getTxCenterFrequency (c), nullptr);
                channelTree.setProperty (propertyAnalogBandwitdh, uhdEngine.getTxBandwidth       (c), nullptr);
            }
        }

        return currentSetup;
    }

    juce::Result UHDEngine::ChannelMapping::deserializeSetup (juce::ValueTree& serializedSetup, UHDEngine& engine)
    {
        Direction direction = static_cast<Direction> (serializedSetup.getType().toString()[0]);

        auto numChannels = serializedSetup.getProperty (propertyNumChannels);
        if (numChannels.isVoid())
        {
            // num_channels property missing
            jassertfalse;
            return juce::Result::fail ("Invalid config file - num_channels property missing");
        }
        const int numChannelsInt = numChannels;

        // we expect the setup to have one child per channel describing the channel setup
        jassert (static_cast<int> (numChannels) == serializedSetup.getNumChildren());

        juce::Array<UHDEngine::ChannelSetup> channelSetup;
        for (int c = 0; c < numChannelsInt; ++c)
        {
            auto channelTree = serializedSetup.getChild (c);
            ChannelSetup channel;

            channel.mboardIdx               = channelTree.getProperty (propertyMboardIdx);
            channel.daughterboardSlot       = channelTree.getProperty (propertyDboardSlot);
            channel.frontendOnDaughterboard = channelTree.getProperty (propertyFrontendOnDboard);
            channel.antennaPort             = channelTree.getProperty (propertyAntennaPort);

            channelSetup.add (std::move (channel));
        }

        juce::Result settingUpChannels                  ((direction == Direction::rx) ? engine.setupRxChannels (channelSetup) : engine.setupTxChannels (channelSetup));
        std::unique_ptr<ChannelMapping>& channelMapping ((direction == Direction::rx) ? engine.rxChannelMapping        : engine.txChannelMapping);

        if (settingUpChannels.failed())
        {
            channelMapping.reset (nullptr);
            return settingUpChannels;
        }

        for (int c = 0; c < numChannelsInt; ++c)
        {
            auto channelTree = serializedSetup.getChild (c);
            const int hardwareChannel = static_cast<int> (channelMapping->bufferOrderToHardwareOrder[c]);

            const int idxOfAnalogGainElement      = channelMapping->gainElementsMap[c][UHDGainElements::analog];
            const int idxOfDigitalGainElement     = channelMapping->gainElementsMap[c][UHDGainElements::digital];
            const int idxOfDigitalFineGainElement = channelMapping->gainElementsMap[c][UHDGainElements::digitalFine];

            UHDr::Error error;

            if (direction == rx)
            {
                if (idxOfAnalogGainElement >= 0)
                {
                    error = engine.usrp->setRxGain (channelTree.getProperty (propertyAnalogGain), hardwareChannel, channelMapping->gainElements[c][idxOfAnalogGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.rxStream.reset (nullptr););
                }

                if (idxOfDigitalGainElement >= 0)
                {
                    error = engine.usrp->setRxGain (channelTree.getProperty (propertyDigitalGain), hardwareChannel, channelMapping->gainElements[c][idxOfDigitalGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.rxStream.reset (nullptr););
                }

                if (idxOfDigitalFineGainElement >= 0)
                {
                    error = engine.usrp->setRxGain (channelTree.getProperty (propertyDigitalGainFine), hardwareChannel, channelMapping->gainElements[c][idxOfDigitalFineGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.rxStream.reset (nullptr););
                }

                auto success = engine.setRxCenterFrequency (channelTree.getProperty (propertyCenterFrequency), c);
                if (!success)
                {
                    channelMapping.reset (nullptr);
                    engine.rxStream.reset (nullptr);
                    return juce::Result::fail ("Error setting Rx center frequency of " + channelTree.getProperty (propertyCenterFrequency).toString () + " for channel " + juce::String (c));
                }

                success = engine.setRxBandwidth (channelTree.getProperty (propertyAnalogBandwitdh), c);
                if (!success)
                {
                    channelMapping.reset (nullptr);
                    engine.rxStream.reset (nullptr);
                    return juce::Result::fail ("Error setting Rx bandwidth of " + channelTree.getProperty (propertyAnalogBandwitdh).toString () + " for channel " + juce::String (c));
                }
            }
            else
            {
                if (idxOfAnalogGainElement >= 0)
                {
                    error = engine.usrp->setTxGain (channelTree.getProperty (propertyAnalogGain), hardwareChannel, channelMapping->gainElements[c][idxOfAnalogGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.txStream.reset (nullptr););
                }

                if (idxOfDigitalGainElement >= 0)
                {
                    error = engine.usrp->setTxGain (channelTree.getProperty (propertyDigitalGain), hardwareChannel, channelMapping->gainElements[c][idxOfDigitalGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.txStream.reset (nullptr););
                }

                if (idxOfDigitalFineGainElement >= 0)
                {
                    error = engine.usrp->setTxGain (channelTree.getProperty (propertyDigitalGainFine), hardwareChannel, channelMapping->gainElements[c][idxOfDigitalFineGainElement].toRawUTF8());
                    NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, channelMapping.reset (nullptr); engine.txStream.reset (nullptr););
                }

                auto success = engine.setTxCenterFrequency (channelTree.getProperty (propertyCenterFrequency), c);
                if (!success)
                {
                    channelMapping.reset (nullptr);
                    engine.txStream.reset (nullptr);
                    return juce::Result::fail ("Error setting Tx center frequency of " + channelTree.getProperty (propertyCenterFrequency).toString () + " for channel " + juce::String (c));
                }

                success = engine.setTxBandwidth (channelTree.getProperty (propertyAnalogBandwitdh), c);
                if (!success)
                {
                    channelMapping.reset (nullptr);
                    engine.txStream.reset (nullptr);
                    return juce::Result::fail ("Error setting Tx bandwith of " + channelTree.getProperty (propertyAnalogBandwitdh).toString () + " for channel " + juce::String (c));
                }
            }
        }
        return juce::Result::ok ();
    }

    const char UHDEngine::ChannelMapping::emptyGainElementString[1] = "";

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

#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK

        if (rxStream != nullptr)
            rxBuffer.reset (new CLSampleBufferComplex<float> (numRxChannels, maxBlockSize, clQueue, clContext, false, CL_MEM_READ_ONLY, CL_MAP_WRITE));
        else
            rxBuffer.reset (new CLSampleBufferComplex<float> (0, 0, clQueue, clContext));

        if (txStream != nullptr)
            txBuffer.reset (new CLSampleBufferComplex<float> (numTxChannels, maxBlockSize, clQueue, clContext, false, CL_MEM_WRITE_ONLY, CL_MAP_READ));
        else
            txBuffer.reset (new CLSampleBufferComplex<float> (0, 0, clQueue, clContext));
#else
        if (rxStream != nullptr)
            rxBuffer.reset (new SampleBufferComplex<float> (numRxChannels, maxBlockSize));
        else
            rxBuffer.reset (new SampleBufferComplex<float> (0, 0));

        if (txStream != nullptr)
            txBuffer.reset (new SampleBufferComplex<float> (numTxChannels, maxBlockSize));
        else
            txBuffer.reset (new SampleBufferComplex<float> (0, 0));
#endif

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
                // the slave device
                usrp->setClockSource ("mimo", 1);
                usrp->setTimeSource  ("mimo", 1);

                // the master device
                usrp->setTimeNow (0, 0.0, 0);

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

            auto issueStreamCmd = rxStream->issueStreamCmd (streamCmd);
            if (issueStreamCmd.failed())
            {
                activeCallback->handleError (issueStreamCmd.getErrorMessage() + ". Stopping stream.");
                activeCallback->streamingHasStopped();
                return;
            }
        }

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

#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
			// Always map buffers at the end of the processRFSampleBlock callback!
			jassert (rxBuffer->isCurrentlyMapped());
			jassert (txBuffer->isCurrentlyMapped());
#endif

            if (txWasEnabled)
            {
                UHDr::Error error;
                auto numSamplesToSend = txBuffer->getNumSamples();
                auto numSamplesSent = txStream->send (txBuffer->getArrayOfWritePointers(), numSamplesToSend, error, 0.5);

                if (error)
                {
                    activeCallback->handleError ("Error executing UHDr::USRP::'TxStream::send: " + UHDr::errorDescription (error) + ". Stopping stream.");
                    activeCallback->streamingHasStopped();
                    return;
                }

                if (numSamplesSent != numSamplesToSend)
                {
                    activeCallback->handleError ("Error sending samples: " + txStream->getLastError());
                }
            }
        }

        if (txStream != nullptr)
        {
            auto error = txStream->sendEndOfBurst();
            if (error)
            {
                activeCallback->handleError ("Warning: Error sending TX endOfBurst flag. " + UHDr::errorDescription (error));
            }

        }

        activeCallback->streamingHasStopped();
    }

    juce::ValueTree UHDEngine::getUHDTree()
    {
        // find all devices
        juce::StringArray allIPAddresses;
        auto allDevices = uhdr->findAllDevices ("");
        for (auto& d : allDevices)
            allIPAddresses.add (d.getValue ("addr", "0.0.0.0"));

        juce::ValueTree tree (propertyUSRPDevice);

        juce::StringArray probeArgs ("uhd_usrp_probe");

        // iterate over all devices
        int currentDevice = 0;
        juce::Array<juce::ValueTree> treeHistory (tree);

        for (auto& a : allIPAddresses)
        {
            // go back to the trees root
            TreeLevel lastLevel = device;
            probeArgs.set (1, "--args=addr=" + a);

            juce::ChildProcess uhdUSRPProbe;
            bool success = uhdUSRPProbe.start (probeArgs);

            if (!success)
                continue;

            auto outputToParse = uhdUSRPProbe.readAllProcessOutput ();

            if (outputToParse.isEmpty ())
                continue;

            auto linesToParse = juce::StringArray::fromLines (outputToParse);

            int idxOfDeviceEntry = 0;
            for (auto& l : linesToParse)
            {
                if (l.contains ("Device: "))
                    break;

                idxOfDeviceEntry++;
            }
            if (idxOfDeviceEntry < linesToParse.size ())
            {
                linesToParse.set (idxOfDeviceEntry, linesToParse[idxOfDeviceEntry] + juce::String (currentDevice));
                currentDevice++;
            }

            for (auto& line : linesToParse)
            {
                // remove whitespaces from the start
                line = line.trimStart();

                if (line.startsWith ("|") || line.startsWith ("/"))
                {
                    // count "|" characters to get the depth
                    TreeLevel level = beforeOrAfterTree;
                    const char* endOfLine = line.toRawUTF8() + line.length ();
                    for (char* c = (char*) line.toRawUTF8(); c != endOfLine; ++c)
                    {
                        if (*c == '|')
                        {
                            level = static_cast<TreeLevel> (level + 1);
                        }
                        else if (*c != ' ')
                        {
                            break;
                        }
                    }

                    // the end of a branch is reached
                    if (level < lastLevel)
                    {
                        if (!((level == device) && (lastLevel == mboard)))
                        {
                            int levelDiff = lastLevel - level;
                            treeHistory.removeLast (levelDiff);
                            if (treeHistory.isEmpty())
                            {
                                lastLevel = device;
                                treeHistory.add (tree);
                            }

                            lastLevel = level;
                            continue;
                        }
                    }

                    // clear up the line
                    line = line.trimCharactersAtStart ("| _/");

                    // just jump to the next line if it's empty now
                    if (line.isEmpty ())
                        continue;

                    // create a pair of property and value
                    auto valuePair = juce::StringArray::fromTokens (line, ":", "");

                    // special case: mac-address has multiple ':' chars leading to unintended string splits
                    if (valuePair[0].equalsIgnoreCase ("mac-addr"))
                        valuePair.set (1, line.fromFirstOccurrenceOf ("mac-addr: ", false, true));

                    // remove whitespaces from value
                    valuePair.set (1, valuePair[1].trimStart());

                    if (level == device)
                    {
                        juce::ValueTree deviceTree (valuePair[1].replaceCharacter (' ', '_').removeCharacters ("/"));
                        treeHistory.getLast().addChild (deviceTree, -1, nullptr);
                        treeHistory.add (deviceTree);
                        lastLevel = mboard;
                        continue;
                    }
                    else
                    {
                        if (level > lastLevel)
                        {
                            auto newLeaf = treeHistory.getLast().getOrCreateChildWithName (valuePair[0].replaceCharacter (' ', '_'), nullptr);

                            // check if the second argument starts with a number. Add a _ to create a valid xml version
                            auto cp = valuePair[1].getCharPointer();
                            if ((*cp >= '0') && (*cp <= '9'))
                                valuePair.set (1, '_' + valuePair[1]);

                            treeHistory.add (newLeaf.getOrCreateChildWithName (valuePair[1].replaceCharacter (' ', '_'), nullptr));
                        }
                        else
                        {
                            // check if it's a range.
                            if (valuePair[1].contains (" to "))
                            {
                                juce::String lowerLimit = valuePair[1].upToFirstOccurrenceOf (" to ", false, false);
                                juce::String remainigPart = valuePair[1].fromFirstOccurrenceOf (" to ", false, false);
                                juce::String upperLimit, step;
                                if (remainigPart.contains (" step "))
                                {
                                    upperLimit   = remainigPart.upToFirstOccurrenceOf (" step ", false, false);
                                    remainigPart = remainigPart.fromFirstOccurrenceOf (" step ", false, false);
                                    step = remainigPart.upToFirstOccurrenceOf (" ", false, false);
                                }
                                else
                                    upperLimit = remainigPart.upToFirstOccurrenceOf (" ", false, false);

                                const double lowerLimitDouble = lowerLimit.getDoubleValue();
                                const double upperLimitDouble = upperLimit.getDoubleValue();

                                // the current limit is unknown at this point. Only exception: There is no choice ;)
                                double currentValue = std::numeric_limits<double>::quiet_NaN();
                                if (lowerLimit == upperLimit)
                                    currentValue = upperLimitDouble;

                                juce::String unit = remainigPart.fromFirstOccurrenceOf (" ", false, false);
                                double stepWidth = 0.0;
                                if (step.isNotEmpty())
                                    stepWidth = step.getDoubleValue();

                                double unitScaling = 1.0;
                                if (unit.startsWithChar ('k'))
                                    unitScaling = 1e3;
                                else if (unit.startsWithChar ('M'))
                                    unitScaling = 1e6;
                                else if (unit.startsWithChar ('G'))
                                    unitScaling = 1e9;

                                juce::ValueTree range (valuePair[0].replaceCharacter (' ', '_'), {{propertyMin, lowerLimitDouble},
                                                                                                  {propertyMax, upperLimitDouble},
                                                                                                  {propertyStepWidth, stepWidth},
                                                                                                  {propertyUnit, unit},
                                                                                                  {propertyUnitScaling, unitScaling},
                                                                                                  {propertyCurrentValue, currentValue}});

                                treeHistory.getLast().addChild (range, -1, nullptr);
                            }
                            else if (valuePair[1].contains (","))
                            {
                                juce::ValueTree array (valuePair[0].replaceCharacter (' ', '_'), {{propertyArray, valuePair[1]},
                                                                                                  {propertyCurrentValue, std::numeric_limits<double>::quiet_NaN()}});
                                treeHistory.getLast().addChild (array, -1, nullptr);
                            }
                            else
                                treeHistory.getLast().setProperty (valuePair[0].replaceCharacter (' ', '_'), valuePair[1], nullptr);
                        }
                        lastLevel = level;
                    }
                }
            }

            lastLevel = device;
            treeHistory.clearQuick();
            treeHistory.add (tree);
        }
#if NTLAB_DEBUGPRINT_UHDTREE
        std::cout << "Parsed UHD tree:\n\n" << tree.toXmlString() << std::endl;
#endif

        return tree;
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

#endif //!JUCE_IOS


#if !JUCE_IOS

    juce::String UHDEngineManager::getEngineName ()
    {
        return "UHD Engine";
    }

    juce::Result UHDEngineManager::isEngineAvailable ()
    {
        if (uhdr != nullptr)
            return juce::Result::ok();

        juce::DynamicLibrary uhdLib;
        if (uhdLib.open (UHDr::uhdLibName))
        {
            uhdLib.close();

            juce::String error;
            uhdr = UHDr::load (UHDr::uhdLibName, error);

            if (uhdr == nullptr)
                return juce::Result::fail (error);

            return juce::Result::ok();
        }

        return juce::Result::fail (UHDr::uhdLibName + " cannot be found on this system");
    }

    SDRIOEngine* UHDEngineManager::createEngine ()
    {
        return new UHDEngine (uhdr);
    }

#endif //!JUCE_IOS
}
