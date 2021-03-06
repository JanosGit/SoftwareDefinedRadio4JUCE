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

#if !JUCE_IOS

#include "UHDReplacement.h"
#include "../../ErrorHandling/ErrorHandlingMacros.h"
#include "../DynamicLibHelpers.h"

namespace ntlab
{
#if JUCE_MAC
	const juce::String UHDr::uhdLibName ("libuhd.dylib");
#elif JUCE_WINDOWS
	const juce::String UHDr::uhdLibName ("C:\\Program Files\\UHD\\bin\\uhd.dll");
#else
	const juce::String UHDr::uhdLibName ("libuhd.so");
#endif
	
	UHDr::UHDSetter::UHDSetter () : numArguments (0) {}

    UHDr::UHDSetter::UHDSetter (UHDr::SetGain fptr, UHDr::USRPHandle usrpHandle, double gain, size_t channel, const char* gainElement)
      : numArguments (4),
        containsDouble (true),
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
         containsDouble (false),
         functionPointer ((void*)fptr),
         firstArg (usrpHandle),
         thirdArg (channel)
    {
        secondArg.asTuneRequestPtr = tuneRequest;
        fourthArg.asTuneResultPtr  = tuneResult;
    }

    UHDr::UHDSetter::UHDSetter (UHDr::SetAntenna fptr, UHDr::USRPHandle usrpHandle, const char* antennaName, size_t channel)
      :  numArguments (3),
         containsDouble (false),
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
         containsDouble (true),
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
                if (containsDouble)
                {
                    auto threeArgFunction = (ThreeArgFunctionSecondArgIsDouble)functionPointer;
                    return threeArgFunction (firstArg, secondArg.asDouble, thirdArg);
                }
                else
                {
                    auto threeArgFunction = (ThreeArgFunctionSecondArgIsPtr)functionPointer;
                    return threeArgFunction (firstArg, secondArg.asVoidPrt, thirdArg);
                }
            }
            case 4:
            {
                if (containsDouble)
                {
                    auto fourArgFunction = (FourArgFunctionSecondArgIsDouble) functionPointer;
                    return fourArgFunction (firstArg, secondArg.asDouble, thirdArg, fourthArg.asVoidPtr);
                }
                else
                {
                    auto fourArgFunction = (FourArgFunctionSecondArgIsPtr) functionPointer;
                    return fourArgFunction (firstArg, secondArg.asVoidPrt, thirdArg, fourthArg.asVoidPtr);
                }
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
            LoadingError<UHDr> loadingError (functionName, result);

            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, usrpMake,                 "uhd_usrp_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, usrpFree,                 "uhd_usrp_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxStreamerMake,           "uhd_rx_streamer_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxStreamerFree,           "uhd_rx_streamer_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxMetadataMake,           "uhd_rx_metadata_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxMetadataFree,           "uhd_rx_metadata_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txStreamerMake,           "uhd_tx_streamer_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txStreamerFree,           "uhd_tx_streamer_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txMetadataMake,           "uhd_tx_metadata_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txMetadataFree,           "uhd_tx_metadata_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txMetadataLastError,      "uhd_tx_metadata_last_error")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, subdevSpecMake,           "uhd_subdev_spec_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, subdevSpecFree,           "uhd_subdev_spec_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, stringVectorMake,         "uhd_string_vector_make")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, stringVectorFree,         "uhd_string_vector_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, stringVectorSize,         "uhd_string_vector_size")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, stringVectorAt,           "uhd_string_vector_at")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, find,                     "uhd_usrp_find")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getNumRxChannels,         "uhd_usrp_get_rx_num_channels")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getNumTxChannels,         "uhd_usrp_get_tx_num_channels")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxSampleRate,          "uhd_usrp_set_rx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxSampleRate,          "uhd_usrp_get_rx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxSampleRate,          "uhd_usrp_set_tx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxSampleRate,          "uhd_usrp_get_tx_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxGain,                "uhd_usrp_set_rx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxGain,                "uhd_usrp_get_rx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxGain,                "uhd_usrp_set_tx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxGain,                "uhd_usrp_get_tx_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxGainElementNames,    "uhd_usrp_get_rx_gain_names");
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxGainElementNames,    "uhd_usrp_get_tx_gain_names");
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxFrequency,           "uhd_usrp_set_rx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxFrequency,           "uhd_usrp_get_rx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxFrequency,           "uhd_usrp_set_tx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxFrequency,           "uhd_usrp_get_tx_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxBandwidth,           "uhd_usrp_set_rx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxBandwidth,           "uhd_usrp_get_rx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxBandwidth,           "uhd_usrp_set_tx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxBandwidth,           "uhd_usrp_get_tx_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxAntenna,             "uhd_usrp_set_rx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxAntenna,             "uhd_usrp_get_rx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxAntennas,            "uhd_usrp_get_rx_antennas")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxAntenna,             "uhd_usrp_set_tx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxAntenna,             "uhd_usrp_get_tx_antenna")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxAntennas,            "uhd_usrp_get_tx_antennas")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setRxSubdevSpec,          "uhd_usrp_set_rx_subdev_spec")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTxSubdevSpec,          "uhd_usrp_set_tx_subdev_spec")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setClockSource,           "uhd_usrp_set_clock_source")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTimeSource,            "uhd_usrp_set_time_source")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTimeUnknownPPS,        "uhd_usrp_set_time_unknown_pps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, setTimeNow,               "uhd_usrp_set_time_now")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxStream,              "uhd_usrp_get_rx_stream")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxStream,              "uhd_usrp_get_tx_stream")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxStreamMaxNumSamples, "uhd_rx_streamer_max_num_samps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getTxStreamMaxNumSamples, "uhd_tx_streamer_max_num_samps")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxStreamerIssueStreamCmd, "uhd_rx_streamer_issue_stream_cmd")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, rxStreamerReceive,        "uhd_rx_streamer_recv")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, txStreamerSend,           "uhd_tx_streamer_send")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (u, getRxMetadataErrorCode,   "uhd_rx_metadata_error_code")

            // if the code reached this point, all functions were successully loaded. The object may now be used safely
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
        // todo: investigate why usrpFree fails on Mac OS but delete works as expected
        if (usrpHandle != nullptr)
#if JUCE_MAC
            delete usrpHandle;
#else
            uhd->usrpFree (&usrpHandle);
#endif
    }

    juce::Result UHDr::USRP::setRxSampleRate (double newSampleRate, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        Error error = uhd->setRxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTxSampleRate (double newSampleRate, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        Error error = uhd->setTxSampleRate (usrpHandle, newSampleRate, static_cast<size_t >(channelIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        return juce::Result::ok();
    }

    double UHDr::USRP::getRxSampleRate (int channelIdx, ntlab::UHDr::Error& error)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        double sampleRate;
        error = uhd->getRxSampleRate (usrpHandle, static_cast<size_t>(channelIdx), &sampleRate);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, sampleRate);
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
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        double sampleRate;
        error = uhd->getTxSampleRate (usrpHandle, static_cast<size_t>(channelIdx), &sampleRate);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, sampleRate);
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

    UHDr::Error UHDr::USRP::setRxGain (double newGain, int channelIdx, const char* gainElement)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));
        return static_cast<Error> (call (UHDSetter (uhd->setRxGain, usrpHandle, newGain, static_cast<size_t> (channelIdx), gainElement)));
    }

    UHDr::Error UHDr::USRP::setTxGain (double newGain, int channelIdx, const char* gainElement)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));
        return static_cast<Error> (call (UHDSetter (uhd->setTxGain, usrpHandle, newGain, static_cast<size_t> (channelIdx), gainElement)));
    }

    double UHDr::USRP::getRxGain (int channelIdx, ntlab::UHDr::Error& error, const char* gainElement)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        double gain;
        error = uhd->getRxGain (usrpHandle, static_cast<size_t>(channelIdx), gainElement, &gain);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, gain);
    }

    juce::StringArray UHDr::USRP::getValidRxGainElements (int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        StringVectorHandle stringVector;
        Error error  = uhd->stringVectorMake (&stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

        error = uhd->getRxGainElementNames (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

        return uhd->uhdStringVectorToStringArray (stringVector);
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

    juce::StringArray UHDr::USRP::getValidTxGainElements (int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        StringVectorHandle stringVector;
        Error error  = uhd->stringVectorMake (&stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

        error = uhd->getTxGainElementNames (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

        return uhd->uhdStringVectorToStringArray (stringVector);
    }

    double UHDr::USRP::getTxGain (int channelIdx, ntlab::UHDr::Error& error, const char* gainElement)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        double gain;
        error = uhd->getTxGain (usrpHandle, static_cast<size_t>(channelIdx), gainElement, &gain);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, gain);
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

    UHDr::Error UHDr::USRP::setRxFrequency (ntlab::UHDr::TuneRequest& tuneRequest, UHDr::TuneResult& tuneResult, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        return static_cast<Error> (call (UHDSetter (uhd->setRxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult)));
    }

    UHDr::Error UHDr::USRP::setTxFrequency (ntlab::UHDr::TuneRequest& tuneRequest, ntlab::UHDr::TuneResult& tuneResult, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        return static_cast<Error> (call (UHDSetter (uhd->setTxFrequency, usrpHandle, &tuneRequest, static_cast<size_t >(channelIdx), &tuneResult)));
    }

    double UHDr::USRP::getRxFrequency (int channelIdx, ntlab::UHDr::Error& error)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        double freq;
        error = uhd->getRxFrequency (usrpHandle, static_cast<size_t>(channelIdx), &freq);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, freq);
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
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        double freq;
        error = uhd->getTxFrequency (usrpHandle, static_cast<size_t>(channelIdx), &freq);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, freq);
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

    UHDr::Error UHDr::USRP::setRxBandwidth (double newBandwidth, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        return static_cast<Error> (call(UHDSetter (uhd->setRxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (channelIdx))));
    }

    UHDr::Error UHDr::USRP::setTxBandwidth (double newBandwidth, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        return static_cast<Error> (call(UHDSetter (uhd->setTxBandwidth, usrpHandle, newBandwidth, static_cast<size_t> (channelIdx))));
    }

    double UHDr::USRP::getRxBandwidth (int channelIdx, ntlab::UHDr::Error& error)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        double bandwidth;
        error = uhd->getRxBandwidth (usrpHandle, static_cast<size_t>(channelIdx), &bandwidth);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, bandwidth);
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
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        double bandwidth;
        error = uhd->getTxBandwidth (usrpHandle, static_cast<size_t>(channelIdx), &bandwidth);
        NTLAB_RETURN_MINUS_ONE_AND_PRINT_ERROR_DBG_IF_FAILED_RETURN_VALUE_OTHERWISE (error, bandwidth);
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

    UHDr::Error UHDr::USRP::setRxAntenna (const char* antennaPort, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));
        return  static_cast<Error> (call (UHDSetter (uhd->setRxAntenna ,usrpHandle, antennaPort, static_cast<size_t> (channelIdx))));
    }

    UHDr::Error UHDr::USRP::setTxAntenna (const char* antennaPort, int channelIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));
        return static_cast<Error> (call (UHDSetter (uhd->setTxAntenna, usrpHandle, antennaPort, static_cast<size_t> (channelIdx))));
    }

    juce::String UHDr::USRP::getCurrentRxAntenna (int channelIdx, ntlab::UHDr::Error& error)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));
        const size_t strBufLen = 32;
        char strBuf[strBufLen];

        error = uhd->getRxAntenna (usrpHandle, static_cast<size_t> (channelIdx), strBuf, strBufLen);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return "";)

        return strBuf;
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
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numInputChannels));

        // create a string vector to store the results
        StringVectorHandle stringVector;
        error = uhd->stringVectorMake (&stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

        error = uhd->getRxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

        return uhd->uhdStringVectorToStringArray (stringVector);
    }

    juce::String UHDr::USRP::getCurrentTxAntenna (int channelIdx, ntlab::UHDr::Error& error)
    {
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        const size_t strBufLen = 32;
        char strBuf[strBufLen];

        error = uhd->getTxAntenna (usrpHandle, static_cast<size_t> (channelIdx), strBuf, strBufLen);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return "";)
        return strBuf;
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
        jassert (juce::isPositiveAndNotGreaterThan (channelIdx, numOutputChannels));

        // create a string vector to store the results
        StringVectorHandle stringVector;
        error = uhd->stringVectorMake (&stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, return juce::StringArray();)

        error = uhd->getTxAntennas (usrpHandle, static_cast<size_t> (channelIdx), &stringVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->stringVectorFree (&stringVector); return juce::StringArray();)

        return uhd->uhdStringVectorToStringArray (stringVector);
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
        jassert (juce::isPositiveAndNotGreaterThan (mboardIdx, numMboards));

        Error error = uhd->setClockSource (usrpHandle, clockSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        return juce::Result::ok();
    }

    juce::Result UHDr::USRP::setTimeSource (const juce::String timeSource, int mboardIdx)
    {
        jassert (juce::isPositiveAndNotGreaterThan (mboardIdx, numMboards));

        Error error = uhd->setTimeSource (usrpHandle, timeSource.toRawUTF8(), static_cast<size_t> (mboardIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

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
        jassert (juce::isPositiveAndNotGreaterThan (mboardIdx, numMboards));

        Error error = uhd->setTimeNow (usrpHandle, fullSecs, fracSecs, static_cast<size_t> (mboardIdx));
        NTLAB_RETURN_FAIL_WITH_ERROR_CODE_DESCRIPTION_IN_CASE_OF_ERROR (error);

        return juce::Result::ok();
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

    const std::string& UHDr::USRP::getLastUSRPError ()
    {
	    if (usrpHandle != nullptr)
            return usrpHandle->lastError;

	    static const std::string noUSRP ("Can't display last USRP error - no USRP was created");
        return noUSRP;
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
        errorDescription = [usrpHandle](Error error) {return UHDr::errorDescription (error) + " (" + usrpHandle->lastError + ")"; };

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

    UHDr::USRP::TxStream * UHDr::USRP::makeTxStream (ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error)
    {
        std::unique_ptr<TxStream> txStream (new TxStream (uhd, usrpHandle, streamArgs, error));

        if (error == Error::errorNone)
            return txStream.release();

        return nullptr;
    }

    UHDr::USRP::TxStream::~TxStream ()
    {
        if (txMetadataStartOfBurst != nullptr)
            uhd->txMetadataFree (&txMetadataStartOfBurst);

        if (txMetadataContinous != nullptr)
            uhd->txMetadataFree (&txMetadataContinous);

        if (txMetadataEndOfBurst != nullptr)
            uhd->txMetadataFree (&txMetadataEndOfBurst);

        if (txStreamerHandle != nullptr)
            uhd->txStreamerFree (&txStreamerHandle);
    }

    const int UHDr::USRP::TxStream::getNumActiveChannels ()
    {
        return numActiveChannels;
    }

    const int UHDr::USRP::TxStream::getMaxNumSamplesPerBlock ()
    {
        return static_cast<int> (maxNumSamples);
    }

    int UHDr::USRP::TxStream::send (UHDr::BuffsPtr buffsPtr, int numSamples, UHDr::Error& error, double timeoutInSeconds)
    {
        size_t numSamplesSent;
        error = uhd->txStreamerSend (txStreamerHandle, buffsPtr, static_cast<size_t> (numSamples), &txMetadataHandle, timeoutInSeconds, &numSamplesSent);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, );

        if (txMetadataHandle == txMetadataStartOfBurst)
            txMetadataHandle = txMetadataContinous;

        return static_cast<int> (numSamplesSent);
    }

    UHDr::Error UHDr::USRP::TxStream::sendEndOfBurst ()
    {
        txMetadataHandle = txMetadataEndOfBurst;
        UHDr::Error error;

        send (nullptr, 0, error, 1.0);

        txMetadataHandle = txMetadataStartOfBurst;

        return error;
    }

    UHDr::USRP::TxStream::TxStream (ntlab::UHDr::Ptr uhdr, ntlab::UHDr::USRPHandle& usrpHandle, ntlab::UHDr::StreamArgs& streamArgs, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        errorDescription = [usrpHandle](Error error) {return UHDr::errorDescription (error) + " (" + usrpHandle->lastError + ")"; };

        error = uhd->txStreamerMake (&txStreamerHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txStreamerHandle = nullptr; return;)

        error = uhd->getTxStream (usrpHandle, &streamArgs, txStreamerHandle);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->txStreamerFree (&txStreamerHandle); return;)

        bool startOfBurst = true;
        bool endOfBurst = false;
        error = uhd->txMetadataMake (&txMetadataStartOfBurst, false, 0, 0.1, startOfBurst, endOfBurst); // taken from the ettus tx_samples_c.c example
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txMetadataStartOfBurst = nullptr; uhd->txStreamerFree (&txStreamerHandle); return;)

        startOfBurst = false;
        error = uhd->txMetadataMake (&txMetadataContinous, false, 0, 0.0, startOfBurst, endOfBurst);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txMetadataContinous = nullptr; uhd->txMetadataFree (&txMetadataStartOfBurst); uhd->txStreamerFree (&txStreamerHandle); return;)

        endOfBurst = true;
        error = uhd->txMetadataMake (&txMetadataEndOfBurst, false, 0, 0.0, startOfBurst, endOfBurst);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, txMetadataContinous = nullptr; uhd->txMetadataFree (&txMetadataStartOfBurst); uhd->txMetadataFree (&txMetadataContinous); uhd->txStreamerFree (&txStreamerHandle); return;)

        txMetadataHandle = txMetadataStartOfBurst;

        numActiveChannels = streamArgs.numChannels;
        error = uhd->getTxStreamMaxNumSamples (txStreamerHandle, &maxNumSamples);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, uhd->txStreamerFree (&txStreamerHandle); return;)
    }

    juce::String UHDr::USRP::TxStream::getLastError ()
    {
        const size_t tmpStringSize = 256;
        char tmpString[tmpStringSize];

        uhd->txMetadataLastError(txMetadataHandle, tmpString, tmpStringSize);

        return juce::String::fromUTF8 (tmpString, tmpStringSize);
    }

    UHDr::USRP::USRP (ntlab::UHDr* uhdr, const char* args, ntlab::UHDr::Error& error) : uhd (uhdr)
    {
        errorDescription = [](Error error)
        {
            auto description = UHDr::errorDescription (error);
            if (error == key)
                description += "\n!!Make sure that your hardware is connected properly!!";
            return description;
        };

        error = uhd->usrpMake (&usrpHandle, args);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, usrpHandle = nullptr; return;)

        errorDescription = [this](Error error) {return UHDr::errorDescription (error) + " (" + usrpHandle->lastError + ")"; };

        size_t n;
        error = uhd->getNumRxChannels (usrpHandle, &n);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, errorDescription = [](Error error) {return UHDr::errorDescription (error); }; uhd->usrpFree (&usrpHandle); return;)

        numInputChannels = static_cast<int>(n);
        error = uhd->getNumTxChannels (usrpHandle, &n);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, errorDescription = [](Error error) {return UHDr::errorDescription (error); }; uhd->usrpFree (&usrpHandle); return;)

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

    juce::StringArray UHDr::uhdStringVectorToStringArray (ntlab::UHDr::StringVectorHandle stringVectorHandle)
    {
        size_t numItemsInVector;
        Error error = stringVectorSize (stringVectorHandle, &numItemsInVector);
        NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, stringVectorFree (&stringVectorHandle); return juce::StringArray();)

        juce::StringArray stringArray;
        const size_t strBufLen = 32;
        char strBuf[strBufLen];
        for (size_t i = 0; i < numItemsInVector; i++)
        {
            error = stringVectorAt (stringVectorHandle, i, strBuf, strBufLen);
            NTLAB_PRINT_ERROR_TO_DBG_AND_INVOKE_ACTIONS (error, stringVectorFree (&stringVectorHandle); return juce::StringArray();)
            stringArray.add (juce::String (strBuf));
        }

        stringVectorFree (&stringVectorHandle);

        return stringArray;
    }

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS

    class UHDReplacementTest : public juce::UnitTest
    {
    public:
        UHDReplacementTest () : juce::UnitTest ("UHDReplacement test") {};

        void runTest () override
        {
            juce::DynamicLibrary uhdLib;
            if (uhdLib.open (UHDr::uhdLibName))
            {
                uhdLib.close();

                beginTest ("Dynamically loading of UHD functions");
                juce::String error;
                expect (UHDr::load (UHDr::uhdLibName, error) != nullptr, error);

                logMessage ("Info: Size of UHDSetter struct: " + juce::String(sizeof (UHDr::UHDSetter)) + " bytes");
            }
            else
                logMessage ("Skipping Dynamically loading of UHD functions, UHD library not present on this system");
        }
    };

    static UHDReplacementTest uhdReplacementTest;

#endif // NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
}

#endif