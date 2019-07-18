

#include "HackRFReplacement.h"
#include "../DynamicLibHelpers.h"

namespace ntlab
{
#if JUCE_MAC
    const juce::String HackRFr::hackRFLibName ("libhackrf.dylib");
#elif JUCE_WINDOWS
    #error "Todo: find windows lib name"
#else
	const juce::String HackRFr::hackRFLibName ("libhackrf.so");
#endif

    HackRFr::HackRFr() {};

	HackRFr::~HackRFr()
	{
        exit();
	    hackRFLib.close();
	}

    HackRFr::Ptr HackRFr::load (const juce::String library, juce::String &result)
    {
        Ptr h = new HackRFr();
        if (h->hackRFLib.open (library))
        {
            // successfully opened library
            juce::DynamicLibrary& lib = h->hackRFLib;

            // try loading all functions
            juce::String functionName;
            LoadingError<HackRFr> loadingError (functionName, result);

            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, getErrorName,               "hackrf_error_name")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, init,                       "hackrf_init")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, exit,                       "hackrf_exit")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, getDeviceList,              "hackrf_device_list")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, deviceListOpen,             "hackrf_device_list_open")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, deviceListFree,             "hackrf_device_list_free")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, open,                       "hackrf_open")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, close,                      "hackrf_close")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, startRx,                    "hackrf_start_rx")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, stopRx,                     "hackrf_stop_rx")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, startTx,                    "hackrf_start_tx")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, stopTx,                     "hackrf_stop_tx")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, isStreaming,                "hackrf_is_streaming")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setBasebandFilterBandwidth, "hackrf_set_baseband_filter_bandwidth")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setFreq,                    "hackrf_set_freq")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setFreqExplicit,            "hackrf_set_freq_explicit")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setSampleRate,              "hackrf_set_sample_rate")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setAmpEnabled,              "hackrf_set_amp_enable")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setLNAGain,                 "hackrf_set_lna_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setVGAGain,                 "hackrf_set_vga_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setTxVGAGain,               "hackrf_set_txvga_gain")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, setAntennaPowerEnabled,     "hackrf_set_antenna_enable")
            NTLAB_LOAD_FUNCTION_AND_CHECK_FOR_SUCCESS (h, getUsbBoardIdName,          "hackrf_usb_board_id_name")

            // if the code reached this point, all functions were successully loaded. The object may now be used safely
            result = "Successfully loaded library " + library;

            h->init();

            return h;
        }

            // problems occured opening the library
            result = "Failed to open library " + library;
            return nullptr;
    }

    juce::StringArray HackRFr::findAllDevices ()
    {
        auto* deviceList = getDeviceList();
        juce::StringArray allDevices;

        for (int deviceIdx = 0; deviceIdx < deviceList->devicecount; ++deviceIdx)
            allDevices.add (getDeviceNameFromDeviceList (deviceList, deviceIdx));

        return allDevices;
    }

    std::unique_ptr<HackRFr::HackRF> HackRFr::createDevice (const juce::String& device, HackRFr::Error& error)
    {
        auto* deviceList = getDeviceList();

        for (int deviceIdx = 0; deviceIdx < deviceList->devicecount; ++deviceIdx)
        {
            juce::String deviceAvailable = getDeviceNameFromDeviceList (deviceList, deviceIdx);

            if (deviceAvailable == device)
            {
                Device deviceHandle = nullptr;
                error = deviceListOpen (deviceList, deviceIdx, &deviceHandle);

                if (error == HackRFr::Error::success)
                {
                    return std::unique_ptr<HackRFr::HackRF> (new HackRFr::HackRF (this, deviceHandle));
                }
            }
        }

        error = HackRFr::Error::notFound;
        return nullptr;
    }

    juce::String HackRFr::getDeviceNameFromDeviceList (const ntlab::HackRFr::DeviceList* deviceList, int deviceIdx)
    {
        return juce::String (getUsbBoardIdName (deviceList->usbBoardIds[deviceIdx])) + " " + juce::String (deviceList->serialNumbers[deviceIdx]);
    }

    HackRFr::HackRF::HackRF (HackRFr::Ptr ptr, HackRFr::Device d) : hackrf (ptr), device (d)
    {
        hackrf->setAmpEnabled          (device, 0);
        hackrf->setAntennaPowerEnabled (device, 0);
    }

    HackRFr::HackRF::~HackRF () { hackrf->close (device); }

    HackRFr::Error HackRFr::HackRF::startRx (HackRFr::SampleBlockCallbackFn callback, void* rxContext) { return hackrf->startRx (device, callback, rxContext); }

    HackRFr::Error HackRFr::HackRF::stopRx()                                                           { return hackrf->stopRx (device); }

    HackRFr::Error HackRFr::HackRF::startTx (HackRFr::SampleBlockCallbackFn callback, void* txContext) { return hackrf->startTx (device, callback, txContext); }

    HackRFr::Error HackRFr::HackRF::stopTx()                                                           { return hackrf->stopTx (device); }

    bool HackRFr::HackRF::isStreaming()                                                                { return hackrf->isStreaming (device) == HackRFr::Error::hackrfTrue; }

    HackRFr::Error HackRFr::HackRF::setBasebandFilterBandwidth (const uint32_t bandwidthInHz)          { return hackrf->setBasebandFilterBandwidth (device, bandwidthInHz); }

    HackRFr::Error HackRFr::HackRF::setFreq (const uint64_t freqInHz)                                  { return hackrf->setFreq (device, freqInHz); }

    HackRFr::Error HackRFr::HackRF::setFreqExplicit (const uint64_t ifFreqInHz, const uint64_t loFreqInHz, const HackRFr::RFPathFilter rfPathFilter)
    {
                                                                                                         return hackrf->setFreqExplicit (device, ifFreqInHz, loFreqInHz, rfPathFilter);
    }

    HackRFr::Error HackRFr::HackRF::setSampleRate (const double freqInHz)                              { return hackrf->setSampleRate (device, freqInHz); }

    HackRFr::Error HackRFr::HackRF::setAmpEnabled (const bool enabled)                                 { return hackrf->setAmpEnabled (device, enabled); }

    HackRFr::Error HackRFr::HackRF::setLNAGain (uint32_t gainInDb)                                     { return hackrf->setLNAGain (device, gainInDb); }

    HackRFr::Error HackRFr::HackRF::setVGAGain (uint32_t gainInDb)                                     { return hackrf->setVGAGain (device, gainInDb); }

    HackRFr::Error HackRFr::HackRF::setTxVGAGain (uint32_t gainInDb)                                   { return hackrf->setTxVGAGain (device, gainInDb); }

    HackRFr::Error HackRFr::HackRF::setAntennaPowerEnabled (const bool enabled)                        { return hackrf->setAntennaPowerEnabled (device, enabled); }


#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS

    class HackRfReplacementTest : public juce::UnitTest
    {
    public:
        HackRfReplacementTest () : juce::UnitTest ("HackRfReplacement test") {};

        void runTest () override
        {
            juce::DynamicLibrary hackrfLib;
            if (hackrfLib.open (HackRFr::hackRFLibName))
            {
                hackrfLib.close();

                beginTest ("Dynamically loading of HackRf functions");
                juce::String error;
                expect (HackRFr::load (HackRFr::hackRFLibName, error) != nullptr, error);
            }
            else
                logMessage ("Skipping Dynamically loading of UHD functions, UHD library not present on this system");
        }
    };

    static HackRfReplacementTest hackRfReplacementTest;

#endif // NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
}