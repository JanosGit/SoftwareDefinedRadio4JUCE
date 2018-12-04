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

#include "UHDReplacement.h"

namespace ntlab
{
    juce::String UHDr::errorDescription (ntlab::UHDr::Error error, juce::String functionName)
    {
        if (functionName.length () > 0)
        {
            if (error == errorNone)
                return "No error while calling " + functionName;

            functionName = "Error calling " + functionName + ": ";
        }

        switch (error)
        {
            case invalidDevice:
                return functionName + "Invalid device arguments";

            case index:
                return functionName + "UHD index error";

            case key:
                return functionName + "UHD key error";

            case notImplemented:
                return functionName + "Not implemented";

            case usb:
                return functionName + "UHD USB error";

            case io:
                return functionName + "UHD I/O error";

            case os:
                return functionName + "UHD operating system error";

            case assertion:
                return functionName + "UHD assertion error";

            case lookup:
                return functionName + "UHD lookup error";

            case type:
                return functionName + "UHD type error";

            case value:
                return functionName + "UHD value error";

            case runtime:
                return functionName + "UHD runtime error";

            case environment:
                return functionName + "UHD environment error";

            case system:
                return functionName + "UHD system error";

            case uhdException:
                return functionName + "UHD exception";

            case boostException:
                return functionName + "boost exception";

            case stdException:
                return functionName + "std exception";

            case unknown:
                return functionName + "Unknown exception";

            case errorNone:
                return "No error";

            default:
                return functionName + "Unknown error code";
        }
    }

    UHDr::~UHDr ()
    {
        uhdLib.close();
    }

    UHDr::Ptr UHDr::load (const juce::String library, juce::String& result)
    {
        Ptr u = new UHDr();
        if (u->uhdLib.open (library)) {
            // successfully opened library
            juce::DynamicLibrary &lib = u->uhdLib;

            // try loading all functions
            juce::String functionName;
            LoadingError loadingError (functionName, result);

            u->usrpMake = (USRPMake) lib.getFunction (functionName = "uhd_usrp_make");
            if (u->usrpMake == nullptr)
                return loadingError.lastFunction();

            u->usrpFree = (USRPFree) lib.getFunction (functionName = "uhd_usrp_make");
            if (u->usrpFree == nullptr)
                return loadingError.lastFunction();

            u->rxStreamerMake = (RxStreamerMake) lib.getFunction (functionName = "uhd_rx_streamer_make");
            if (u->rxStreamerMake == nullptr)
                return loadingError.lastFunction();

            u->rxStreamerFree = (RxStreamerFree) lib.getFunction (functionName = "uhd_rx_streamer_free");
            if (u->rxStreamerFree == nullptr)
                return loadingError.lastFunction();

            u->rxMetadataMake = (RxMetadataMake) lib.getFunction (functionName = "uhd_rx_metadata_make");
            if (u->rxMetadataMake == nullptr)
                return loadingError.lastFunction();

            u->rxMetadataFree = (RxMetadataFree) lib.getFunction (functionName = "uhd_rx_metadata_free");
            if (u->rxMetadataFree == nullptr)
                return loadingError.lastFunction();

            u->txStreamerMake = (TxStreamerMake) lib.getFunction (functionName = "uhd_tx_streamer_make");
            if (u->txStreamerMake == nullptr)
                return loadingError.lastFunction();

            u->txStreamerFree = (TxStreamerFree) lib.getFunction (functionName = "uhd_tx_streamer_free");
            if (u->txStreamerFree == nullptr)
                return loadingError.lastFunction();

            u->txMetadataMake = (TxMetadataMake) lib.getFunction (functionName = "uhd_tx_metadata_make");
            if (u->txStreamerMake == nullptr)
                return loadingError.lastFunction();

            u->txMetadataFree = (TxMetadataFree) lib.getFunction (functionName = "uhd_tx_metadata_free");
            if (u->txMetadataFree == nullptr)
                return loadingError.lastFunction();

            u->stringVectorMake = (StringVectorMake) lib.getFunction (functionName = "uhd_string_vector_make");
            if (u->stringVectorMake == nullptr)
                return loadingError.lastFunction();

            u->stringVectorFree = (StringVectorFree) lib.getFunction (functionName = "uhd_string_vector_free");
            if (u->stringVectorFree == nullptr)
                return loadingError.lastFunction();

            u->stringVectorSize = (StringVectorSize) lib.getFunction (functionName = "uhd_string_vector_size");
            if (u->stringVectorSize == nullptr)
                return loadingError.lastFunction();

            u->stringVectorAt = (StringVectorAt) lib.getFunction (functionName = "uhd_string_vector_at");
            if (u->stringVectorAt == nullptr)
                return loadingError.lastFunction();

            u->find = (Find) lib.getFunction (functionName = "uhd_usrp_find");
            if (u->find == nullptr)
                return loadingError.lastFunction();

            u->getNumRxChannels = (GetNumRxChannels) lib.getFunction (functionName = "uhd_usrp_get_rx_num_channels");
            if (u->getNumRxChannels == nullptr)
                return loadingError.lastFunction();

            u->getNumTxChannels = (GetNumTxChannels) lib.getFunction (functionName = "uhd_usrp_get_tx_num_channels");
            if (u->getNumTxChannels == nullptr)
                return loadingError.lastFunction();

            u->setRxSampleRate = (SetSampleRate) lib.getFunction (functionName = "uhd_usrp_set_rx_rate");
            if (u->setRxSampleRate == nullptr)
                return loadingError.lastFunction();

            u->getRxSampleRate = (GetSampleRate) lib.getFunction (functionName = "uhd_usrp_get_rx_rate");
            if (u->getRxSampleRate == nullptr)
                return loadingError.lastFunction();

            u->setTxSampleRate = (SetSampleRate) lib.getFunction (functionName = "uhd_usrp_set_tx_rate");
            if (u->setRxSampleRate == nullptr)
                return loadingError.lastFunction();

            u->getTxSampleRate = (GetSampleRate) lib.getFunction (functionName = "uhd_usrp_get_tx_rate");
            if (u->getRxSampleRate == nullptr)
                return loadingError.lastFunction();

            u->setRxGain = (SetGain) lib.getFunction (functionName = "uhd_usrp_set_rx_gain");
            if (u->setRxGain == nullptr)
                return loadingError.lastFunction();

            u->getRxGain = (GetGain) lib.getFunction (functionName = "uhd_usrp_get_rx_gain");
            if (u->getRxGain == nullptr)
                return loadingError.lastFunction();

            u->setTxGain = (SetGain) lib.getFunction (functionName = "uhd_usrp_set_tx_gain");
            if (u->setTxGain == nullptr)
                return loadingError.lastFunction();

            u->getTxGain = (GetGain) lib.getFunction (functionName = "uhd_usrp_get_tx_gain");
            if (u->getTxGain == nullptr)
                return loadingError.lastFunction();

            u->setRxFrequency = (SetFrequency) lib.getFunction (functionName = "uhd_usrp_set_rx_freq");
            if (u->setRxFrequency == nullptr)
                return loadingError.lastFunction();

            u->getRxFrequency = (GetFrequency) lib.getFunction (functionName = "uhd_usrp_get_rx_freq");
            if (u->getRxFrequency == nullptr)
                return loadingError.lastFunction();

            u->setTxFrequency = (SetFrequency) lib.getFunction (functionName = "uhd_usrp_set_tx_freq");
            if (u->setTxFrequency == nullptr)
                return loadingError.lastFunction();

            u->getTxFrequency = (GetFrequency) lib.getFunction (functionName = "uhd_usrp_get_tx_freq");
            if (u->getTxFrequency == nullptr)
                return loadingError.lastFunction();

            u->setRxBandwidth = (SetBandwidth) lib.getFunction (functionName = "uhd_usrp_set_rx_bandwidth");
            if (u->setRxBandwidth == nullptr)
                return loadingError.lastFunction();

            u->getRxBandwidth = (GetBandwidth) lib.getFunction (functionName = "uhd_usrp_get_rx_bandwidth");
            if (u->getRxBandwidth == nullptr)
                return loadingError.lastFunction();

            u->setTxBandwidth = (SetBandwidth) lib.getFunction (functionName = "uhd_usrp_set_tx_bandwidth");
            if (u->setTxBandwidth == nullptr)
                return loadingError.lastFunction();

            u->getTxBandwidth = (GetBandwidth) lib.getFunction (functionName = "uhd_usrp_get_tx_bandwidth");
            if (u->getTxBandwidth == nullptr)
                return loadingError.lastFunction();

            u->setRxAntenna = (SetAntenna) lib.getFunction (functionName = "uhd_usrp_set_rx_antenna");
            if (u->setRxAntenna == nullptr)
                return loadingError.lastFunction();

            u->getRxAntenna = (GetAntenna) lib.getFunction (functionName = "uhd_usrp_get_rx_antenna");
            if (u->getRxAntenna == nullptr)
                return loadingError.lastFunction();

            u->getRxAntennas = (GetAntennas) lib.getFunction (functionName = "uhd_usrp_get_rx_antennas");
            if (u->getRxAntennas == nullptr)
                return loadingError.lastFunction();

            u->setTxAntenna = (SetAntenna) lib.getFunction (functionName = "uhd_usrp_set_tx_antenna");
            if (u->setTxAntenna == nullptr)
                return loadingError.lastFunction();

            u->getTxAntenna = (GetAntenna) lib.getFunction (functionName = "uhd_usrp_get_tx_antenna");
            if (u->getTxAntenna == nullptr)
                return loadingError.lastFunction();

            u->getTxAntennas = (GetAntennas) lib.getFunction (functionName = "uhd_usrp_get_tx_antennas");
            if (u->getTxAntennas == nullptr)
                return loadingError.lastFunction();

            u->setClockSource = (SetSource) lib.getFunction (functionName = "uhd_usrp_set_clock_source");
            if (u->setClockSource == nullptr)
                return loadingError.lastFunction();

            u->setTimeSource = (SetSource) lib.getFunction (functionName = "uhd_usrp_set_time_source");
            if (u->setTimeSource == nullptr)
                return loadingError.lastFunction();

            u->setTimeUnknownPPS = (SetTimeUnknownPPS) lib.getFunction (functionName = "uhd_usrp_set_time_unknown_pps");
            if (u->setTimeUnknownPPS == nullptr)
                return loadingError.lastFunction();

            u->setTimeNow = (SetTimeNow) lib.getFunction (functionName = "uhd_usrp_set_time_now");
            if (u->setTimeNow == nullptr)
                return loadingError.lastFunction();

            u->getRxStream = (GetRxStream) lib.getFunction (functionName = "uhd_usrp_get_rx_stream");
            if (u->getRxStream == nullptr)
                return loadingError.lastFunction();

            u->getTxStream = (GetTxStream) lib.getFunction (functionName = "uhd_usrp_get_tx_stream");
            if (u->getTxStream == nullptr)
                return loadingError.lastFunction();

            u->getRxStreamMaxNumSamples = (GetRxStreamMaxNumSamples) lib.getFunction (functionName = "uhd_rx_streamer_max_num_samps");
            if (u->getRxStreamMaxNumSamples == nullptr)
                return loadingError.lastFunction();

            u->getTxStreamMaxNumSamples = (GetTxStreamMaxNumSamples) lib.getFunction (functionName = "uhd_tx_streamer_max_num_samps");
            if (u->getTxStreamMaxNumSamples == nullptr)
                return loadingError.lastFunction();

            u->rxStreamerIssueStreamCmd = (RxStreamerIssueStreamCmd) lib.getFunction (functionName = "uhd_rx_streamer_issue_stream_cmd");
            if (u->getRxStreamMaxNumSamples == nullptr)
                return loadingError.lastFunction();

            u->rxStreamerReceive = (RxStreamerReceive) lib.getFunction (functionName = "uhd_rx_streamer_recv");
            if (u->rxStreamerReceive == nullptr)
                return loadingError.lastFunction();

            u->txStreamerSend = (TxStreamerSend) lib.getFunction (functionName = "uhd_tx_streamer_send");
            if (u->txStreamerSend == nullptr)
                return loadingError.lastFunction();

            u->getRxMetadataErrorCode = (GetRxMetadataErrorCode) lib.getFunction (functionName = "uhd_rx_metadata_error_code");
            if (u->getRxMetadataErrorCode == nullptr)
                return loadingError.lastFunction();

            // if the code reached this point, all functions were successully loaded. The object may now be safely constructed
            result = "Successfully loaded library " + library;
            return u;
        }
        else
        {
            // problems occured opening the library
            result = "Failed to open library " + library;
            return nullptr;
        }
    }

    juce::Array<juce::StringPairArray> UHDr::findAllDevices (const juce::String args)
    {
        Error error;

        // create a string vector to store the results
        StringVectorHandle stringVector;
        error = stringVectorMake (&stringVector);
        if (error) {
            DBG (errorDescription (error, "UHDr::stringVectorMake"));
            jassertfalse;
            return juce::StringPairArray();
        }


        // try finding the devices
        error = find (args.toRawUTF8(), &stringVector);
        if (error) {
            DBG (errorDescription (error, "UHDr::find"));
            jassertfalse;
            stringVectorFree (&stringVector);
            return juce::StringPairArray();
        }

        // this number should equal the number of devices found
        size_t numItemsInStringVector;
        error = stringVectorSize (stringVector, &numItemsInStringVector);
        if (error) {
            DBG (errorDescription (error, "UHDr::stringVectorSize"));
            jassertfalse;
            stringVectorFree (&stringVector);
            return juce::StringPairArray();
        }

        // iterate over the whole result and put the key-value-pairs into a StringPairArray per device
        juce::Array<juce::StringPairArray> allDevices;
        const int tempStringBufferLength = 512;
        char tempStringBuffer[tempStringBufferLength];
        for (size_t i = 0; i < numItemsInStringVector; ++i) {
            // load content of string vector into the temporary buffer
            error = stringVectorAt (stringVector, i, tempStringBuffer, tempStringBufferLength);
            if (error) {
                DBG (errorDescription (error, "UHDr::stringVectorAt"));
                jassertfalse;
                stringVectorFree (&stringVector);
                return juce::StringPairArray();
            }

            // tokenize the temporary buffer to extract all key-value-pairs
            juce::StringPairArray pairs;
            auto deviceAttributes = juce::StringArray::fromTokens (tempStringBuffer, ",", "");
            for (auto &d : deviceAttributes) {
                // tokenize the pair again to seperate key and value
                auto pair = juce::StringArray::fromTokens (d, "=", "");
                pairs.set (pair[0], pair[1]);
            }

            allDevices.add (pairs);
        }

        stringVectorFree (&stringVector);

        return allDevices;
    }

    UHDr::USRP::~USRP ()
    {
        if (usrpHandle != nullptr)
            uhd->usrpFree (&usrpHandle);
    }

    juce::Result UHDr::USRP::setRxSampleRate (double newSampleRate, int channelIdx)
    {
        Error error;
        if (channelIdx == -1) {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = uhd->setRxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxSampleRate"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = uhd->setRxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxSampleRate"));
            }
            else {
                return juce::Result::fail ("channelIdx passed out of range");
            }
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxSampleRate (double newSampleRate, int channelIdx)
    {
        Error error;
        if (channelIdx == -1) {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = uhd->setTxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxSampleRate"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = uhd->setTxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxSampleRate"));
            }
            else
            {
                return juce::Result::fail ("channelIdx passed out of range");
            }
        }
        return juce::Result::ok();
    }

    double UHDr::USRP::getRxSampleRate (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            double sampleRate;
            error = uhd->getRxSampleRate (usrpHandle, static_cast<size_t>(channelIdx), &sampleRate);
            if (error)
                return -1.0;
            return sampleRate;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getRxSampleRates (ntlab::UHDr::Error& error)
    {
        juce::Array<double> sampleRates;
        for (int i = 0; i < numInputChannels; i++)
        {
            double sampleRate;
            error = uhd->getRxSampleRate (usrpHandle, static_cast<size_t>(i), &sampleRate);
            if (error)
                return juce::Array<double>();

            sampleRates.add (sampleRate);
        }
        return sampleRates;
    }

    double UHDr::USRP::getTxSampleRate (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            double sampleRate;
            error = uhd->getTxSampleRate (usrpHandle, static_cast<size_t>(channelIdx), &sampleRate);
            if (error)
                return -1.0;
            return sampleRate;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getTxSampleRates (ntlab::UHDr::Error& error)
    {
        juce::Array<double> sampleRates;
        for (int i = 0; i < numOutputChannels; i++)
        {
            double sampleRate;
            error = uhd->getTxSampleRate (usrpHandle, static_cast<size_t>(i), &sampleRate);
            if (error)
                return juce::Array<double>();

            sampleRates.add (sampleRate);
        }
        return sampleRates;
    }

    juce::Result UHDr::USRP::setRxGain (double newGain, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = uhd->setRxGain (usrpHandle, newGain, static_cast<size_t >(i), "");
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxGain"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = uhd->setRxGain (usrpHandle, newGain, static_cast<size_t >(channelIdx), "");
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxGain"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxGain (double newGain, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = uhd->setTxGain (usrpHandle, newGain, static_cast<size_t >(i), "");
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxGain"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = uhd->setTxGain (usrpHandle, newGain, static_cast<size_t >(channelIdx), "");
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxGain"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    double UHDr::USRP::getRxGain (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            double gain;
            error = uhd->getRxGain (usrpHandle, static_cast<size_t>(channelIdx), "", &gain);
            if (error)
                return -1.0;
            return gain;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getRxGains (ntlab::UHDr::Error& error)
    {
        juce::Array<double> gains;
        for (int i = 0; i < numInputChannels; i++)
        {
            double gain;
            error = uhd->getRxGain (usrpHandle, static_cast<size_t>(i), "", &gain);
            if (error)
                return juce::Array<double>();
            gains.add (gain);
        }
        return gains;
    }

    double UHDr::USRP::getTxGain (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            double gain;
            error = uhd->getTxGain (usrpHandle, static_cast<size_t>(channelIdx), "", &gain);
            if (error)
                return -1.0;
            return gain;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getTxGains (ntlab::UHDr::Error& error)
    {
        juce::Array<double> gains;
        for (int i = 0; i < numOutputChannels; i++)
        {
            double gain;
            error = uhd->getTxGain (usrpHandle, static_cast<size_t>(i), "", &gain);
            if (error)
                return juce::Array<double>();
            gains.add (gain);
        }
        return gains;
    }

    juce::Result UHDr::USRP::setRxFrequency (ntlab::UHDr::TuneRequest& tuneRequest, juce::Array<ntlab::UHDr::TuneResult>& tuneResults, int channelIdx)
    {
        tuneResults.clearQuick();
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numInputChannels; i++)
            {
                TuneResult tuneResult;
                error = uhd->setRxFrequency (usrpHandle, &tuneRequest, static_cast<size_t >(i), &tuneResult);
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxFrequency"));
                tuneResults.add (tuneResult);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                TuneResult tuneResult;
                error = uhd->setRxFrequency (usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult);
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxFrequency"));
                tuneResults.add (tuneResult);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxFrequency (ntlab::UHDr::TuneRequest& tuneRequest, juce::Array<ntlab::UHDr::TuneResult>& tuneResults, int channelIdx)
    {
        tuneResults.clearQuick();
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                TuneResult tuneResult;
                error = uhd->setTxFrequency (usrpHandle, &tuneRequest, static_cast<size_t >(i), &tuneResult);
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxFrequency"));
                tuneResults.add (tuneResult);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                TuneResult tuneResult;
                error = uhd->setTxFrequency (usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult);
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxFrequency"));
                tuneResults.add (tuneResult);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    double UHDr::USRP::getRxFrequency (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            double freq;
            error = uhd->getRxFrequency (usrpHandle, static_cast<size_t>(channelIdx), &freq);
            if (error)
                return -1.0;
            return freq;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getRxFrequencies (ntlab::UHDr::Error& error)
    {
        juce::Array<double> frequencies;
        for (int i = 0; i < numInputChannels; i++)
        {
            double freq;
            error = uhd->getRxFrequency (usrpHandle, static_cast<size_t>(i), &freq);
            if (error)
                return juce::Array<double>();
            frequencies.add (freq);
        }
        return frequencies;
    }

    double UHDr::USRP::getTxFrequency (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            double freq;
            error = uhd->getTxFrequency (usrpHandle, static_cast<size_t>(channelIdx), &freq);
            if (error)
                return -1.0;
            return freq;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getTxFrequencies (ntlab::UHDr::Error& error)
    {
        juce::Array<double> frequencies;
        for (int i = 0; i < numOutputChannels; i++)
        {
            double freq;
            error = uhd->getTxFrequency (usrpHandle, static_cast<size_t>(i), &freq);
            if (error)
                return juce::Array<double>();
            frequencies.add (freq);
        }
        return frequencies;
    }

    juce::Result UHDr::USRP::setRxBandwidth (double newBandwidth, int channelIdx)
    {
        Error error;
        if (channelIdx == -1) {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = uhd->setRxBandwidth (usrpHandle, newBandwidth, static_cast<size_t > (i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxBandwidth"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = uhd->setRxBandwidth (usrpHandle, newBandwidth, static_cast<size_t >(channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxBandwidth"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxBandwidth (double newBandwidth, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = uhd->setTxBandwidth (usrpHandle, newBandwidth, static_cast<size_t >(i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxBandwidth"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = uhd->setTxBandwidth (usrpHandle, newBandwidth, static_cast<size_t >(channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxBandwidth"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    double UHDr::USRP::getRxBandwidth (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            double bandwidth;
            error = uhd->getRxBandwidth (usrpHandle, static_cast<size_t>(channelIdx), &bandwidth);
            if (error)
                return -1.0;
            return bandwidth;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getRxBandwidths (ntlab::UHDr::Error& error)
    {
        juce::Array<double> bandwidths;
        for (int i = 0; i < numInputChannels; i++)
        {
            double bandwidh;
            error = uhd->getRxBandwidth (usrpHandle, static_cast<size_t>(i), &bandwidh);
            if (error)
                return juce::Array<double>();
            bandwidths.add (bandwidh);
        }
        return bandwidths;
    }

    double UHDr::USRP::getTxBandwidth (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            double bandwidth;
            error = uhd->getTxBandwidth (usrpHandle, static_cast<size_t>(channelIdx), &bandwidth);
            if (error)
                return -1.0;
            return bandwidth;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return -1.0;
    }

    juce::Array<double> UHDr::USRP::getTxBandwidths (ntlab::UHDr::Error& error)
    {
        juce::Array<double> bandwidths;
        for (int i = 0; i < numOutputChannels; i++)
        {
            double bandwidh;
            error = uhd->getTxBandwidth (usrpHandle, static_cast<size_t>(i), &bandwidh);
            if (error)
                return juce::Array<double>();
            bandwidths.add (bandwidh);
        }
        return bandwidths;
    }

    juce::Result UHDr::USRP::setRxAntenna (const juce::String antennaPort, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = uhd->setRxAntenna (usrpHandle, antennaPort.toRawUTF8(), static_cast<size_t> (i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxAntenna"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = uhd->setRxAntenna (usrpHandle, antennaPort.toRawUTF8(), static_cast<size_t> (channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setRxAntenna"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxAntenna (const juce::String antennaPort, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = uhd->setTxAntenna (usrpHandle, antennaPort.toRawUTF8(), static_cast<size_t> (i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxAntenna"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = uhd->setTxAntenna (usrpHandle, antennaPort.toRawUTF8(), static_cast<size_t> (channelIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTxAntenna"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::String UHDr::USRP::getCurrentRxAntenna (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            const size_t strBufLen = 32;
            char strBuf[strBufLen];

            error = uhd->getRxAntenna (usrpHandle, static_cast<size_t> (channelIdx), strBuf, strBufLen);
            if (error)
                return "";
            return strBuf;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return "";
    }

    juce::StringArray UHDr::USRP::getCurrentRxAntennas (ntlab::UHDr::Error& error)
    {
        juce::StringArray currentAntennas;
        const size_t strBufLen = 32;
        char strBuf[strBufLen];
        for (int i = 0; i < numInputChannels; i++)
        {
            error = uhd->getRxAntenna (usrpHandle, static_cast<size_t> (i), strBuf, strBufLen);
            if (error)
                return juce::StringArray();
            currentAntennas.add (juce::String (strBuf));
        }
        return currentAntennas;
    }

    juce::StringArray UHDr::USRP::getPossibleRxAntennas (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            // create a string vector to store the results
            StringVectorHandle stringVector;
            error = uhd->stringVectorMake (&stringVector);
            if (error)
                return juce::StringArray();

            error = uhd->getRxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
            if (error)
            {
                uhd->stringVectorFree (&stringVector);
                return juce::StringArray();
            }

            size_t numItemsInVector;
            error = uhd->stringVectorSize (stringVector, &numItemsInVector);
            if (error)
            {
                uhd->stringVectorFree (&stringVector);
                return juce::StringArray();
            }

            juce::StringArray possibleAntennas;
            const size_t strBufLen = 32;
            char strBuf[strBufLen];
            for (size_t i = 0; i < numItemsInVector; i++)
            {
                error = uhd->stringVectorAt (stringVector, i, strBuf, strBufLen);
                if (error)
                {
                    uhd->stringVectorFree (&stringVector);
                    return juce::StringArray();
                }
                possibleAntennas.add (juce::String (strBuf));
            }

            error = uhd->stringVectorFree (&stringVector);
            return possibleAntennas;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return juce::StringArray();
    }

    juce::String UHDr::USRP::getCurrentTxAntenna (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            const size_t strBufLen = 32;
            char strBuf[strBufLen];

            error = uhd->getTxAntenna (usrpHandle, static_cast<size_t> (channelIdx), strBuf, strBufLen);
            if (error)
                return "";
            return strBuf;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return "";
    }

    juce::StringArray UHDr::USRP::getCurrentTxAntennas (ntlab::UHDr::Error& error)
    {
        juce::StringArray currentAntennas;
        const size_t strBufLen = 32;
        char strBuf[strBufLen];
        for (int i = 0; i < numOutputChannels; i++)
        {
            error = uhd->getTxAntenna (usrpHandle, static_cast<size_t> (i), strBuf, strBufLen);
            if (error)
                return juce::StringArray();
            currentAntennas.add (juce::String (strBuf));
        }
        return currentAntennas;
    }

    juce::StringArray UHDr::USRP::getPossibleTxAntennas (int channelIdx, ntlab::UHDr::Error& error)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            // create a string vector to store the results
            StringVectorHandle stringVector;
            error = uhd->stringVectorMake (&stringVector);
            if (error)
                return juce::StringArray();

            error = uhd->getTxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
            if (error)
            {
                uhd->stringVectorFree (&stringVector);
                return juce::StringArray();
            }

            size_t numItemsInVector;
            error = uhd->stringVectorSize (stringVector, &numItemsInVector);
            if (error)
            {
                uhd->stringVectorFree (&stringVector);
                return juce::StringArray();
            }

            juce::StringArray possibleAntennas;
            const size_t strBufLen = 32;
            char strBuf[strBufLen];
            for (size_t i = 0; i < numItemsInVector; i++)
            {
                error = uhd->stringVectorAt (stringVector, i, strBuf, strBufLen);
                if (error)
                {
                    uhd->stringVectorFree (&stringVector);
                    return juce::StringArray();
                }
                possibleAntennas.add (juce::String (strBuf));
            }

            error = uhd->stringVectorFree (&stringVector);
            return possibleAntennas;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return juce::StringArray();
    }

    juce::Result UHDr::USRP::setClockSource (const juce::String clockSource, int mboardIdx)
    {
        Error error;
        if (mboardIdx == -1)
        {
            for (int i = 0; i < numMboards; i++)
            {
                error = uhd->setClockSource (usrpHandle, clockSource.toRawUTF8(), static_cast<size_t> (i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setClockSource"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (numMboards, numOutputChannels))
            {
                error = uhd->setClockSource (usrpHandle, clockSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setClockSource"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeSource (const juce::String timeSource, int mboardIdx)
    {
        Error error;
        if (mboardIdx == -1)
        {
            for (int i = 0; i < numMboards; i++)
            {
                error = uhd->setTimeSource (usrpHandle, timeSource.toRawUTF8(), static_cast<size_t> (i));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTimeSource"));
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (numMboards, numOutputChannels))
            {
                error = uhd->setTimeSource (usrpHandle, timeSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
                if (error)
                    return juce::Result::fail (errorDescription (error, "UHDr::setTimeSource"));
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeUnknownPPS (time_t fullSecs, double fracSecs)
    {
        Error error = uhd->setTimeUnknownPPS (usrpHandle, fullSecs, fracSecs);
        if (error)
            return juce::Result::fail (errorDescription (error, "UHDr::setTimeUnknownPPS"));
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeNow (time_t fullSecs, double fracSecs, int mboardIdx)
    {
        if (juce::isPositiveAndNotGreaterThan (mboardIdx, numMboards))
        {
            Error error = uhd->setTimeNow (usrpHandle, fullSecs, fracSecs, static_cast<size_t> (mboardIdx));
            if (error)
                return juce::Result::fail (errorDescription (error, "UHDr::setTimeNow"));
            return juce::Result::ok();
        }
        // don't use this to configure all mboards!
        jassertfalse;
        return juce::Result::fail ("channelIdx passed out of range");
    }

    const int UHDr::USRP::getNumInputChannels ()
    {
        return numInputChannels;
    }

    const int UHDr::USRP::getNumOutputChannels ()
    {
        return numOutputChannels;
    }

    UHDr::USRP::RxStream::~RxStream ()
    {
        if (rxMetadataHandle != nullptr)
            uhd->rxMetadataFree (&rxMetadataHandle);

        if (rxStreamerHandle != nullptr)
            uhd->rxStreamerFree (&rxStreamerHandle);
    }

    const int UHDr::USRP::RxStream::getNumActiveChannels ()
    {
        return numActiveChannels;
    }

    const int UHDr::USRP::RxStream::getMaxNumSamplesPerBlock ()
    {
        return static_cast<int> (maxNumSamples);
    }

    juce::Result UHDr::USRP::RxStream::issueStreamCmd (ntlab::UHDr::StreamCmd& streamCmd)
    {
        Error error = uhd->rxStreamerIssueStreamCmd (rxStreamerHandle, &streamCmd);
        if (error)
            return juce::Result::fail (errorDescription (error, "RxStream::issueStreamCmd"));
        return juce::Result::ok();
    }

    int UHDr::USRP::RxStream::receive (ntlab::UHDr::BuffsPtr buffsPtr, int numSamples, ntlab::UHDr::Error& error, bool onePacket, double timeoutInSeconds)
    {
        size_t numSamplesReceived;
        error = uhd->rxStreamerReceive (rxStreamerHandle, buffsPtr, static_cast<size_t> (numSamples), &rxMetadataHandle, timeoutInSeconds, onePacket, &numSamplesReceived);
        return static_cast<int> (numSamplesReceived);
    }

    UHDr::RxMetadataError UHDr::USRP::RxStream::getLastRxMetadataError (ntlab::UHDr::Error& error)
    {
        RxMetadataError lastRxMetadataError;
        error = uhd->getRxMetadataErrorCode (rxMetadataHandle, &lastRxMetadataError);
        return lastRxMetadataError;
    }

    UHDr::USRP::RxStream::RxStream (ntlab::UHDr::Ptr uhdr, ntlab::UHDr::USRPHandle& usrpHandle, ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        error = uhd->rxStreamerMake (&rxStreamerHandle);
        if (error) {
            rxStreamerHandle = nullptr;
            return;
        }

        error = uhd->getRxStream (usrpHandle, &streamArgs, rxStreamerHandle);
        if (error)
            return;

        error = uhd->rxMetadataMake (&rxMetadataHandle);
        if (error) {
            rxMetadataHandle = nullptr;
            return;
        }

        numActiveChannels = streamArgs.numChannels;
        error = uhd->getRxStreamMaxNumSamples (rxStreamerHandle, &maxNumSamples);
    }

    UHDr::USRP::RxStream * UHDr::USRP::makeRxStream (ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error)
    {
        std::unique_ptr<RxStream> rxStream (new RxStream (uhd, usrpHandle, streamArgs, error));

        if (error == Error::errorNone)
            return rxStream.release();

        return nullptr;
    }

    UHDr::USRP::TxStream::~TxStream ()
    {
        if (txMetadataHandle != nullptr)
            uhd->rxMetadataFree (&txMetadataHandle);

        if (txStreamerHandle != nullptr)
            uhd->rxStreamerFree (&txStreamerHandle);
    }

    const int UHDr::USRP::TxStream::getNumActiveChannels ()
    {
        return numActiveChannels;
    }

    const int UHDr::USRP::TxStream::getMaxNumSamplesPerBlock ()
    {
        return static_cast<int> (maxNumSamples);
    }

    juce::Result UHDr::USRP::TxStream::issueStreamCmd (ntlab::UHDr::StreamCmd& streamCmd)
    {
        Error error = uhd->txStreamerIssueStreamCmd (txStreamerHandle, &streamCmd);
        if (error)
            return juce::Result::fail (errorDescription (error, "TxStream::issueStreamCmd"));
        return juce::Result::ok();
    }

    int UHDr::USRP::TxStream::send (ntlab::UHDr::BuffsPtr buffsPtr, int numSamples, ntlab::UHDr::Error& error, double timeoutInSeconds)
    {
        size_t numSamplesSent;
        error = uhd->txStreamerSend (txStreamerHandle, buffsPtr, static_cast<size_t> (numSamples), &txMetadataHandle, timeoutInSeconds, &numSamplesSent);
        return static_cast<int> (numSamplesSent);
    }

    UHDr::USRP::TxStream::TxStream (ntlab::UHDr::Ptr uhdr, ntlab::UHDr::USRPHandle& usrpHandle, ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        error = uhd->txStreamerMake (&txStreamerHandle);
        if (error) {
            txStreamerHandle = nullptr;
            return;
        }

        error = uhd->getTxStream (usrpHandle, &streamArgs, txStreamerHandle);
        if (error)
            return;

        error = uhd->txMetadataMake (&txMetadataHandle, false, 0, 0.1, true, false); // taken from the ettus tx_samples_c.c example
        if (error) {
            txMetadataHandle = nullptr;
            return;
        }

        numActiveChannels = streamArgs.numChannels;
        error = uhd->getTxStreamMaxNumSamples (txStreamerHandle, &maxNumSamples);
    }

    UHDr::USRP::USRP (ntlab::UHDr* uhdr, const char* args, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        error = uhd->usrpMake (&usrpHandle, args);
        if (error == Error::errorNone)
        {
            size_t n;
            error = uhd->getNumRxChannels (usrpHandle, &n);
            if (error == Error::errorNone)
            {
                numInputChannels = static_cast<int>(n);
                error = uhd->getNumTxChannels (usrpHandle, &n);
                numOutputChannels = static_cast<int>(n);
            }
        }
        else
            usrpHandle = nullptr;
    }

    UHDr::USRP::Ptr UHDr::makeUSRP (juce::StringPairArray args, ntlab::UHDr::Error& error)
    {
        juce::String argsSingleString = args.getDescription().removeCharacters (" ");

        USRP::Ptr usrp = new USRP (this, argsSingleString.toRawUTF8(), error);

        if (error)
            return nullptr;

        // todo: only safe if nothing more than ip adresses are passed with the args. Find better solution!
        usrp->numMboards = args.size();

        return usrp;
    }
}