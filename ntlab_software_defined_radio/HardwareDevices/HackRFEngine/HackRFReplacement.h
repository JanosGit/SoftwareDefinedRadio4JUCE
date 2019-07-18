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

#include <juce_core/juce_core.h>

namespace ntlab
{

    class HackRFr : public juce::ReferenceCountedObject
    {
    public:

        typedef juce::ReferenceCountedObjectPtr<HackRFr> Ptr;

        enum Error
        {
            success             = 0,
            hackrfTrue          = 1,
            invalidParam        = -2,
            notFound            = -5,
            busy                = -6,
            noMem               = -11,
            libusb              = -1000,
            thread              = -1001,
            streamingThread     = -1002,
            streamingStopped    = -1003,
            streamingExitCalled = -1004,
            usbApiVersion       = -1005,
            notLastDevice       = -2000,
            other               = -9999
        };

        enum BoardID
        {
            jellybean  = 0,
            jawbreaker = 1,
            hackrfOne = 2,
            RAD1O = 3,
            invalid = 0xFF
        };

        enum USBBoardID
        {
            usbJawbreaker = 0x604B,
            usbHackrfOne  = 0x6089,
            usbRAD1O      = 0xCC15,
            usbInvalid    = 0xFFFF
        };

        enum RFPathFilter
        {
            bypass   = 0,
            lowPass  = 1,
            highPass = 2
        };

        enum SweepStyle
        {
            linear      = 0,
            interleaved = 1
        };

        typedef void* Device;

        struct Transfer
        {
            Device  device;
            int8_t* buffer; // the original api has an unsigned type here, however signed values are expected...
            int     bufferLength;
            int     validLength;
            void*   rxContext;
            void*   txContext;
        };

        struct DeviceList
        {
            char**      serialNumbers;
            USBBoardID* usbBoardIds;
            int*        usbDeviceIndex;
            int         devicecount;

            void**      usb_devices;
            int         usb_devicecount;
        };

        typedef int (*SampleBlockCallbackFn)(Transfer*);

        typedef int         (*InitExit)();
        typedef DeviceList* (*GetDeviceList)();
        typedef Error       (*DeviceListOpen)            (DeviceList*, int, Device*);
        typedef void        (*DeviceListFree)            (DeviceList*);
        typedef Error       (*Open)                      (Device*);
        typedef Error       (*Close)                     (Device);
        typedef Error       (*Start)                     (Device, SampleBlockCallbackFn, void*);
        typedef Error       (*Stop)                      (Device);
        typedef Error       (*IsStreaming)               (Device);
        typedef Error       (*SetBasebandFilterBandwidth)(Device, const uint32_t);
        typedef Error       (*SetFreq)                   (Device, const uint64_t);
        typedef Error       (*SetFreqExplicit)           (Device, const uint64_t, const uint64_t, const RFPathFilter);
        typedef Error       (*SetSampleRate)             (Device, const double);
        typedef Error       (*SetGain)                   (Device, uint32_t);
        typedef Error       (*SetEnabled)                (Device, const uint8_t);
        typedef const char* (*GetErrorName)              (Error);
        typedef const char* (*GetUSBBoardIDName)         (USBBoardID);

        static const juce::String hackRFLibName;

        ~HackRFr();

        static Ptr load (const juce::String library, juce::String &result);

        GetErrorName getErrorName;

        juce::StringArray findAllDevices();

        class HackRF
        {
            friend class HackRFr;
        public:

            Error startRx (SampleBlockCallbackFn callback, void* rxContext);

            Error stopRx();

            Error startTx (SampleBlockCallbackFn callback, void* txContext);

            Error stopTx();

            bool isStreaming();

            Error setBasebandFilterBandwidth (const uint32_t bandwidthInHz);

            Error setFreq (const uint64_t freqInHz);

            Error setFreqExplicit (const uint64_t ifFreqInHz, const uint64_t loFreqInHz, const RFPathFilter rfPathFilter);

            Error setSampleRate (const double freqInHz);

            Error setAmpEnabled (const bool enabled);

            Error setLNAGain (uint32_t gainInDb);

            Error setVGAGain (uint32_t gainInDb);

            Error setTxVGAGain (uint32_t gainInDb);

            Error setAntennaPowerEnabled (const bool enabled);

            ~HackRF();

        private:
            HackRFr::Ptr    hackrf;
            HackRFr::Device device;

            HackRF (HackRFr::Ptr ptr, HackRFr::Device d);
        };

        std::unique_ptr<HackRF> createDevice (const juce::String& device, Error& error);

    private:

        InitExit                   init;
        InitExit                   exit;
        GetDeviceList              getDeviceList;
        DeviceListOpen             deviceListOpen;
        DeviceListFree             deviceListFree;
        Open                       open;
        Close                      close;
        Start                      startRx;
        Stop                       stopRx;
        Start                      startTx;
        Stop                       stopTx;
        IsStreaming                isStreaming;
        SetBasebandFilterBandwidth setBasebandFilterBandwidth;
        SetFreq                    setFreq;
        SetFreqExplicit            setFreqExplicit;
        SetSampleRate              setSampleRate;
        SetEnabled                 setAmpEnabled;
        SetGain                    setLNAGain;
        SetGain                    setVGAGain;
        SetGain                    setTxVGAGain;
        SetEnabled                 setAntennaPowerEnabled;
        GetUSBBoardIDName          getUsbBoardIdName;


        juce::DynamicLibrary hackRFLib;

        HackRFr();

        juce::String getDeviceNameFromDeviceList (const DeviceList* deviceList, int deviceIdx);
    };
}