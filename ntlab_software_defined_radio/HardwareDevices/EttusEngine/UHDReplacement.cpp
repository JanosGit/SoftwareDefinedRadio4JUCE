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
#include "../../ErrorHandling/ErrorHandlingMacros.h"

namespace ntlab
{
    UHDr::UHDSetter::UHDSetter () : numArguments (0) {}

    UHDr::UHDSetter::UHDSetter (UHDr::SetGain fptr, UHDr::USRPHandle usrpHandle, double gain, size_t channel, const char* gainElement)
      : numArguments (4),
        functionPointer ((void*)fptr),
        firstArg (usrpHandle),
        thirdArg (channel)
    {
        secondArg.asDouble = gain;
        fourthArg.asCharPtr = stringBuffer;
        // If this assert is hit a gain element name with a length > 6 characters was used.
        // The string buffer size should be adjusted if a string of such length is needed
        jassert (std::strlen (gainElement) < stringBufferSize);
        std::strncpy (stringBuffer, gainElement, stringBufferSize);
    }

    UHDr::UHDSetter::UHDSetter (UHDr::SetFrequency fptr, UHDr::USRPHandle usrpHandle, UHDr::TuneRequest* tuneRequest, size_t channel, UHDr::TuneResult* tuneResult)
      :  numArguments (4),
         functionPointer ((void*)fptr),
         firstArg (usrpHandle),
         thirdArg (channel)
    {
        secondArg.asTuneRequestPtr = tuneRequest;
        fourthArg.asTuneResultPtr  = tuneResult;
    }

    UHDr::UHDSetter::UHDSetter (UHDr::SetAntenna fptr, UHDr::USRPHandle usrpHandle, const char* antennaName, size_t channel)
      :  numArguments (3),
         functionPointer ((void*)fptr),
         firstArg (usrpHandle),
         thirdArg (channel)
    {
        secondArg.asCharPtr = stringBuffer;
        // If this assert is hit a antenna name with a length > 6 characters was used.
        // The string buffer size should be adjusted if a string of such length is needed
        jassert (std::strlen (antennaName) < stringBufferSize);
        std::strncpy (stringBuffer, antennaName, stringBufferSize);
    }

    UHDr::UHDSetter::UHDSetter (ntlab::UHDr::SetBandwidth fptr, ntlab::UHDr::USRPHandle usrpHandle, double bandwidth, size_t channel)
      :  numArguments (3),
         functionPointer ((void*)fptr),
         firstArg (usrpHandle),
         thirdArg (channel)
    {
        secondArg.asDouble = bandwidth;
    }

    int UHDr::UHDSetter::invoke() const
    {
        switch (numArguments)
        {
            case 3:
            {
                auto threeArgFunction = (ThreeArgFunction)functionPointer;
                return threeArgFunction (firstArg, secondArg, thirdArg);
            }
            case 4:
            {
                auto fourArgFunction = (FourArgFunction)functionPointer;
                return fourArgFunction (firstArg, secondArg, thirdArg, fourthArg);
            }
            default:
                return Error::unknown;
        }
    }

    void* UHDr::UHDSetter::getErrorContext() {return functionPointer; }

    juce::String UHDr::errorDescription (ntlab::UHDr::Error error)
    {
        switch (error)
        {
            case invalidDevice:
                return "Invalid device arguments";

            case index:
                return "UHD index error";

            case key:
                return "UHD key error";

            case notImplemented:
                return "Not implemented";

            case usb:
                return "UHD USB error";

            case io:
                return "UHD I/O error";

            case os:
                return "UHD operating system error";

            case assertion:
                return "UHD assertion error";

            case lookup:
                return "UHD lookup error";

            case type:
                return "UHD type error";

            case value:
                return "UHD value error";

            case runtime:
                return "UHD runtime error";

            case environment:
                return "UHD environment error";

            case system:
                return "UHD system error";

            case uhdException:
                return "UHD exception";

            case boostException:
                return "boost exception";

            case stdException:
                return "std exception";

            case unknown:
                return "Unknown exception";

            case realtimeCallFIFO:
                return "Realtime setter fifo is full";

            case errorNone:
                return "No error";

            default:
                return "Unknown error code";
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
            juce::DynamicLibrary& lib = u->uhdLib;

            // try loading all functions
            juce::String functionName;
            LoadingError loadingError (functionName, result);

            // Define a macro to avoid a lot of repetitive boilerplate code
#define NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS(fptr, fptrType, fnName) u->fptr = (fptrType) lib.getFunction (functionName = fnName); \
                                                                          if (u->fptr == nullptr) \
                                                                              return loadingError.lastFunction();

            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (usrpMake,                 USRPMake,                 "uhd_usrp_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (usrpFree,                 USRPFree,                 "uhd_usrp_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxStreamerMake,           RxStreamerMake,           "uhd_rx_streamer_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxStreamerFree,           RxStreamerFree,           "uhd_rx_streamer_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxMetadataMake,           RxMetadataMake,           "uhd_rx_metadata_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxMetadataFree,           RxMetadataFree,           "uhd_rx_metadata_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (txStreamerMake,           TxStreamerMake,           "uhd_tx_streamer_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (txStreamerFree,           TxStreamerFree,           "uhd_tx_streamer_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (txMetadataMake,           TxMetadataMake,           "uhd_tx_metadata_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (txMetadataFree,           TxMetadataFree,           "uhd_tx_metadata_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (subdevSpecMake,           SubdevSpecMake,           "uhd_subdev_spec_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (subdevSpecFree,           SubdevSpecFree,           "uhd_subdev_spec_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (stringVectorMake,         StringVectorMake,         "uhd_string_vector_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (stringVectorFree,         StringVectorFree,         "uhd_string_vector_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (stringVectorSize,         StringVectorSize,         "uhd_string_vector_size")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (stringVectorAt,           StringVectorAt,           "uhd_string_vector_at")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (find,                     Find,                     "uhd_usrp_find")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getNumRxChannels,         GetNumRxChannels,         "uhd_usrp_get_rx_num_channels")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getNumTxChannels,         GetNumTxChannels,         "uhd_usrp_get_tx_num_channels")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxSampleRate,          SetSampleRate,            "uhd_usrp_set_rx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxSampleRate,          GetSampleRate,            "uhd_usrp_get_rx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxSampleRate,          SetSampleRate,            "uhd_usrp_set_tx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxSampleRate,          GetSampleRate,            "uhd_usrp_get_tx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxGain,                SetGain,                  "uhd_usrp_set_rx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxGain,                GetGain,                  "uhd_usrp_get_rx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxGain,                SetGain,                  "uhd_usrp_set_tx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxGain,                GetGain,                  "uhd_usrp_get_tx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxFrequency,           SetFrequency,             "uhd_usrp_set_rx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxFrequency,           GetFrequency,             "uhd_usrp_get_rx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxFrequency,           SetFrequency,             "uhd_usrp_set_tx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxFrequency,           GetFrequency,             "uhd_usrp_get_tx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxBandwidth,           SetBandwidth,             "uhd_usrp_set_rx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxBandwidth,           GetBandwidth,             "uhd_usrp_get_rx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxBandwidth,           SetBandwidth,             "uhd_usrp_set_tx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxBandwidth,           GetBandwidth,             "uhd_usrp_get_tx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxAntenna,             SetAntenna,               "uhd_usrp_set_rx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxAntenna,             GetAntenna,               "uhd_usrp_get_rx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxAntennas,            GetAntennas,              "uhd_usrp_get_rx_antennas")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxAntenna,             SetAntenna,               "uhd_usrp_set_tx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxAntenna,             GetAntenna,               "uhd_usrp_get_tx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxAntennas,            GetAntennas,              "uhd_usrp_get_tx_antennas")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setRxSubdevSpec,          SetSubdevSpec,            "uhd_usrp_set_rx_subdev_spec")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTxSubdevSpec,          SetSubdevSpec,            "uhd_usrp_set_tx_subdev_spec")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setClockSource,           SetSource,                "uhd_usrp_set_clock_source")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTimeSource,            SetSource,                "uhd_usrp_set_time_source")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTimeUnknownPPS,        SetTimeUnknownPPS,        "uhd_usrp_set_time_unknown_pps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (setTimeNow,               SetTimeNow,               "uhd_usrp_set_time_now")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxStream,              GetRxStream,              "uhd_usrp_get_rx_stream")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxStream,              GetTxStream,              "uhd_usrp_get_tx_stream")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxStreamMaxNumSamples, GetRxStreamMaxNumSamples, "uhd_rx_streamer_max_num_samps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getTxStreamMaxNumSamples, GetTxStreamMaxNumSamples, "uhd_tx_streamer_max_num_samps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxStreamerIssueStreamCmd, RxStreamerIssueStreamCmd, "uhd_rx_streamer_issue_stream_cmd")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (rxStreamerReceive,        RxStreamerReceive,        "uhd_rx_streamer_recv")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (txStreamerSend,           TxStreamerSend,           "uhd_tx_streamer_send")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (getRxMetadataErrorCode,   GetRxMetadataErrorCode,   "uhd_rx_metadata_error_code")

#undef NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS

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
        jassert (error == Error::errorNone);

        // try finding the devices
        error = find (args.toRawUTF8(), &stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, stringVectorFree (&stringVector); return juce::StringPairArray();)


        // this number should equal the number of devices found
        size_t numItemsInStringVector;
        error = stringVectorSize (stringVector, &numItemsInStringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, stringVectorFree (&stringVector); return juce::StringPairArray();)

        // iterate over the whole result and put the key-value-pairs into a StringPairArray per device
        juce::Array<juce::StringPairArray> allDevices;
        const int tempStringBufferLength = 512;
        char tempStringBuffer[tempStringBufferLength];
        for (size_t i = 0; i < numItemsInStringVector; ++i) {
            // load content of string vector into the temporary buffer
            error = stringVectorAt (stringVector, i, tempStringBuffer, tempStringBufferLength);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, stringVectorFree (&stringVector); return juce::StringPairArray();)

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
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = uhd->setRxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = uhd->setTxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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

    juce::Result UHDr::USRP::setRxGain (double newGain, int channelIdx, const char* gainElement)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setRxGain, usrpHandle, newGain, static_cast<size_t >(i), gainElement)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setRxGain, usrpHandle, newGain, static_cast<size_t >(channelIdx), gainElement)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxGain (double newGain, int channelIdx, const char* gainElement)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setTxGain, usrpHandle, newGain, static_cast<size_t >(i), gainElement)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setTxGain, usrpHandle, newGain, static_cast<size_t >(channelIdx), gainElement)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    double UHDr::USRP::getRxGain (int channelIdx, ntlab::UHDr::Error& error, const char* gainElement)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
        {
            double gain;
            error = uhd->getRxGain (usrpHandle, static_cast<size_t>(channelIdx), gainElement, &gain);
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

    double UHDr::USRP::getTxGain (int channelIdx, ntlab::UHDr::Error& error, const char* gainElement)
    {
        if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
        {
            double gain;
            error = uhd->getTxGain (usrpHandle, static_cast<size_t>(channelIdx), gainElement, &gain);
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
                error = static_cast<Error> (call (UHDSetter (uhd->setRxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(i), &tuneResult)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
                tuneResults.add (tuneResult);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                TuneResult tuneResult;
                error = static_cast<Error> (call (UHDSetter (uhd->setRxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
                error = static_cast<Error> (call (UHDSetter (uhd->setTxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(i), &tuneResult)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
                tuneResults.add (tuneResult);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                TuneResult tuneResult;
                error = static_cast<Error> (call (UHDSetter (uhd->setTxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult)));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
                error = static_cast<Error> (call(UHDSetter (uhd->setRxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (i))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = static_cast<Error> (call(UHDSetter (uhd->setRxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (channelIdx))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
                error = static_cast<Error> (call(UHDSetter (uhd->setTxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (i))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = static_cast<Error> (call(UHDSetter (uhd->setTxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (channelIdx))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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

    juce::Result UHDr::USRP::setRxAntenna (const char* antennaPort, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numInputChannels; i++)
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setRxAntenna ,usrpHandle, antennaPort, static_cast<size_t> (i))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels))
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setRxAntenna ,usrpHandle, antennaPort, static_cast<size_t> (channelIdx))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxAntenna (const char* antennaPort, int channelIdx)
    {
        Error error;
        if (channelIdx == -1)
        {
            for (int i = 0; i < numOutputChannels; i++)
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setTxAntenna ,usrpHandle, antennaPort, static_cast<size_t> (i))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels))
            {
                error = static_cast<Error> (call (UHDSetter (uhd->setTxAntenna ,usrpHandle, antennaPort, static_cast<size_t> (channelIdx))));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)
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
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

            error = uhd->getRxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

            size_t numItemsInVector;
            error = uhd->stringVectorSize (stringVector, &numItemsInVector);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

            juce::StringArray possibleAntennas;
            const size_t strBufLen = 32;
            char strBuf[strBufLen];
            for (size_t i = 0; i < numItemsInVector; i++)
            {
                error = uhd->stringVectorAt (stringVector, i, strBuf, strBufLen);
                NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)
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
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return "";)
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
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)
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
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

            error = uhd->getTxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

            size_t numItemsInVector;
            error = uhd->stringVectorSize (stringVector, &numItemsInVector);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

            juce::StringArray possibleAntennas;
            const size_t strBufLen = 32;
            char strBuf[strBufLen];
            for (size_t i = 0; i < numItemsInVector; i++)
            {
                error = uhd->stringVectorAt (stringVector, i, strBuf, strBufLen);
                NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)
                possibleAntennas.add (juce::String (strBuf));
            }

            uhd->stringVectorFree (&stringVector);
            return possibleAntennas;
        }
        // This assert will fire if you passed an invalid channelIdx. Dont pass -1
        jassertfalse;
        error = Error::unknown;
        return juce::StringArray();
    }

    juce::Result UHDr::USRP::setRxSubdevSpec (const juce::String& subdevSpec, int mboardIdx)
    {
        jassert (juce::isPositiveAndBelow (mboardIdx, numMboards));

        Error error;
        SubdevSpecHandle subdevSpecHandle;
        error = uhd->subdevSpecMake (&subdevSpecHandle, subdevSpec.toRawUTF8());
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        uhd->setRxSubdevSpec (usrpHandle, subdevSpecHandle, static_cast<size_t> (mboardIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, uhd->subdevSpecFree (&subdevSpecHandle); )

        uhd->subdevSpecFree (&subdevSpecHandle);

        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxSubdevSpec (const juce::String& subdevSpec, int mboardIdx)
    {
        jassert (juce::isPositiveAndBelow (mboardIdx, numMboards));

        Error error;
        SubdevSpecHandle subdevSpecHandle;
        error = uhd->subdevSpecMake (&subdevSpecHandle, subdevSpec.toRawUTF8());
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        uhd->setTxSubdevSpec (usrpHandle, subdevSpecHandle, static_cast<size_t> (mboardIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR_AND_INVOKE_ACTIONS_BEFORE (error, uhd->subdevSpecFree (&subdevSpecHandle); )

        uhd->subdevSpecFree (&subdevSpecHandle);

        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setClockSource (const juce::String clockSource, int mboardIdx)
    {
        Error error;
        if (mboardIdx == -1)
        {
            for (int i = 0; i < numMboards; i++)
            {
                error = uhd->setClockSource (usrpHandle, clockSource.toRawUTF8(), static_cast<size_t> (i));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (numMboards, numOutputChannels))
            {
                error = uhd->setClockSource (usrpHandle, clockSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
        }
        else
        {
            if (juce::isPositiveAndNotGreaterThan (numMboards, numOutputChannels))
            {
                error = uhd->setTimeSource (usrpHandle, timeSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
                NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
            }
            else
                return juce::Result::fail ("channelIdx passed out of range");
        }
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeUnknownPPS (time_t fullSecs, double fracSecs)
    {
        Error error = uhd->setTimeUnknownPPS (usrpHandle, fullSecs, fracSecs);
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeNow (time_t fullSecs, double fracSecs, int mboardIdx)
    {
        if (juce::isPositiveAndNotGreaterThan (mboardIdx, numMboards))
        {
            Error error = uhd->setTimeNow (usrpHandle, fullSecs, fracSecs, static_cast<size_t> (mboardIdx));
            NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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

    const int UHDr::USRP::getNumMboards ()
    {
        return numMboards;
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
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);
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
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, rxStreamerHandle = nullptr; return;)

        error = uhd->getRxStream (usrpHandle, &streamArgs, rxStreamerHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->rxStreamerFree (&rxStreamerHandle); return;)

        error = uhd->rxMetadataMake (&rxMetadataHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, rxMetadataHandle = nullptr; uhd->rxStreamerFree (&rxStreamerHandle); return;)

        numActiveChannels = streamArgs.numChannels;
        error = uhd->getRxStreamMaxNumSamples (rxStreamerHandle, &maxNumSamples);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->rxMetadataFree (&rxMetadataHandle); uhd->rxStreamerFree (&rxStreamerHandle); return;)
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

    juce::Result UHDr::USRP::TxStream::issueStreamCmd (UHDr::StreamCmd& streamCmd)
    {
        Error error = uhd->txStreamerIssueStreamCmd (txStreamerHandle, &streamCmd);
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        return juce::Result::ok();
    }

    int UHDr::USRP::TxStream::send (UHDr::BuffsPtr buffsPtr, int numSamples, UHDr::Error& error, double timeoutInSeconds)
    {
        size_t numSamplesSent;
        error = uhd->txStreamerSend (txStreamerHandle, buffsPtr, static_cast<size_t> (numSamples), &txMetadataHandle, timeoutInSeconds, &numSamplesSent);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, );
        return static_cast<int> (numSamplesSent);
    }

    UHDr::USRP::TxStream::TxStream (ntlab::UHDr::Ptr uhdr, ntlab::UHDr::USRPHandle& usrpHandle, ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        error = uhd->txStreamerMake (&txStreamerHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txStreamerHandle = nullptr; return;)

        error = uhd->getTxStream (usrpHandle, &streamArgs, txStreamerHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->txStreamerFree (&txStreamerHandle); return;)

        error = uhd->txMetadataMake (&txMetadataHandle, false, 0, 0.1, true, false); // taken from the ettus tx_samples_c.c example
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txMetadataHandle = nullptr; uhd->txStreamerFree (&txStreamerHandle); return;)

        numActiveChannels = streamArgs.numChannels;
        error = uhd->getTxStreamMaxNumSamples (txStreamerHandle, &maxNumSamples);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->txStreamerFree (&txStreamerHandle); return;)
    }

    UHDr::USRP::USRP (ntlab::UHDr* uhdr, const char* args, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        error = uhd->usrpMake (&usrpHandle, args);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, usrpHandle = nullptr; return;)

        size_t n;
        error = uhd->getNumRxChannels (usrpHandle, &n);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->usrpFree (&usrpHandle); return;)

        numInputChannels = static_cast<int>(n);
        error = uhd->getNumTxChannels (usrpHandle, &n);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->usrpFree (&usrpHandle); return;)

        numOutputChannels = static_cast<int>(n);
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

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS

    class UHDReplacementTest : public juce::UnitTest
    {
    public:
        UHDReplacementTest () : juce::UnitTest ("UHDReplacement test") {};

        void runTest () override
        {
#if JUCE_MAC
            juce::String uhdLibName = "libuhd.dylib";
#endif
            juce::DynamicLibrary uhdLib;
            if (uhdLib.open (uhdLibName))
            {
                uhdLib.close();

                beginTest ("Dynamically loading of UHD functions");
                juce::String error;
                expect (UHDr::load (uhdLibName, error) != nullptr, error);

                logMessage ("Info: Size of UHDSetter struct: " + juce::String(sizeof (UHDr::UHDSetter)) + " bytes");
            }
            else
                logMessage ("Skipping Dynamically loading of UHD functions, UHD library not present on this system");
        }
    };

    static UHDReplacementTest uhdReplacementTest;

#endif // NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
}