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
#include "../../Threading/RealtimeSetterThreadWithFIFO.h"
#include <complex>

namespace ntlab
{
    /**
 * This class is used to access all relevant functions of the Ettus UHD C-API and allows to access them trough a
 * safe object orientated and JUCE inspired API. It's not dependent on any UHD header and tries to act as an
 * extremely thin wrapper for all performance-critical functions while translating some configuration and information
 * request into JUCE-based datatypes.
 *
 * I decided to implement it this way to keep the modules codebase as self-containig and small as possible.
 *
 * Thank goes out to Ettus for building such incredible good hard- and Software. Visit www.ettus.com for more
 * intormation about the USRP SDR devices and the UHD library.
 */
    class UHDr : public juce::ReferenceCountedObject
    {
    public:

        typedef juce::ReferenceCountedObjectPtr<UHDr> Ptr;

        enum Error : int
        {
            //! No error thrown.
                    errorNone           = 0,
            //! Invalid device arguments.
                    invalidDevice       = 1,
            //! See uhd::index_error.
                    index               = 10,
            //! See uhd::key_error.
                    key                 = 11,
            //! See uhd::not_implemented_error.
                    notImplemented      = 20,
            //! See uhd::usb_error.
                    usb                 = 21,
            //! See uhd::io_error.
                    io                  = 30,
            //! See uhd::os_error.
                    os                  = 31,
            //! See uhd::assertion_error.
                    assertion           = 40,
            //! See uhd::lookup_error.
                    lookup              = 41,
            //! See uhd::type_error.
                    type                = 42,
            //! See uhd::value_error.
                    value               = 43,
            //! See uhd::runtime_error.
                    runtime             = 44,
            //! See uhd::environment_error.
                    environment         = 45,
            //! See uhd::system_error.
                    system              = 46,
            //! See uhd::exception.
                    uhdException        = 47,
            //! A boost::exception was thrown.
                    boostException      = 60,
            //! A std::exception was thrown.
                    stdException        = 70,
            //! An unknown error was thrown.
                    unknown             = 100,
            //! NTLAB specific: The call was invoked from the realtime thread and therefore pushed to the FIFO where
            //  something failed
                    realtimeCallFIFO    = -1
        };

        /**
         * This function translates UHD API error codes into a human readable string representation. You can optionally
         * pass a function name that was called before so that the error description mentions the function where the
         * error occured.
         */
        static juce::String errorDescription (Error error);

        enum RxMetadataError
        {
            //! No error code associated with this metadata
                    rxMetadataErrorNone   = 0x0,
            //! No packet received, implementation timed out
                    rxTimeout             = 0x1,
            //! A stream command was issued in the past
                    rxLateCommand         = 0x2,
            //! Expected another stream command
                    rxBrokenChain         = 0x4,
            //! Overflow or sequence error
                    rxCodeOverflow        = 0x8,
            //! Multi-channel alignment failed
                    rxCodeAlignment       = 0xC,
            //! The packet could not be parsed
                    rxBadPacket           = 0xF
        };

        enum StreamMode
        {
            //! Stream samples indefinitely
                    startContinuous     = 97,
            //! End continuous streaming
                    stopContinuous      = 111,
            //! Stream some number of samples and finish
                    numSampsAndDone     = 100,
            //! Stream some number of samples but expect more
                    numSampsAndMore     = 109
        };

        enum TuneRequestPolicy
        {
            //! Do not set this argument, use current setting.
                    DONT_USE_none       = 78,
            //! Automatically determine the argument's value.
                    automatic           = 65,
            //! Use the argument's value for the setting.
                    manual              = 77
        };

        struct TuneRequest
        {
            //! Target frequency for RF chain in Hz
            double targetFreq;
            //! RF frequency policy
            TuneRequestPolicy rfFreqPolicy = TuneRequestPolicy::automatic;
            //! RF frequency in Hz
            double rf_freq;
            //! DSP frequency policy
            TuneRequestPolicy dspFreqPolicy = TuneRequestPolicy::automatic;
            //! DSP frequency in Hz
            double dspFreq;
            //! Key-value pairs delimited by commas
            char* args;
        };

        struct TuneResult
        {
            //! Target RF frequency, clipped to be within system range
            double clippedRfFreq;
            //! Target RF frequency, including RF FE offset
            double targetRfFreq;
            //! Frequency to which RF LO is actually tuned
            double actualRfFreq;
            //! Frequency the CORDIC must adjust the RF
            double targetDspFreq;
            //! Frequency to which the CORDIC in the DSP actually tuned
            double actualDspFreq;
        };

        struct StreamArgs
        {
            //! Format of host memory
            char* cpuFormat;
            //! Over-the-wire format
            char* otwFormat;
            //! Other stream args
            char* args;
            //! Array that lists channels
            size_t* channelList;
            //! Number of channels
            int numChannels;
        };

        struct StreamCmd
        {
            //! How streaming is issued to the device
            StreamMode streamMode;
            //! Number of samples
            size_t numSamples;
            //! Stream now?
            bool streamNow;
            //! If not now, then full seconds into future to stream
            time_t timeSpecFullSecs;
            //! If not now, then fractional seconds into future to stream
            double timeSpecFracSecs;
        };

        struct USRPstruct
        {
            size_t usrpIndex;
            std::string lastError;
        };

        typedef std::complex<float>** BuffsPtr;

        // construction and deletion of handles
        typedef USRPstruct* USRPHandle;
        typedef Error (*USRPMake)(USRPHandle*, const char*);
        typedef Error (*USRPFree)(USRPHandle*);

        typedef void* RxStreamerHandle;
        typedef Error (*RxStreamerMake)(RxStreamerHandle*);
        typedef Error (*RxStreamerFree)(RxStreamerHandle*);

        typedef void* RxMetadataHandle;
        typedef Error (*RxMetadataMake)(RxMetadataHandle*);
        typedef Error (*RxMetadataFree)(RxMetadataHandle*);

        typedef void* TxStreamerHandle;
        typedef Error (*TxStreamerMake)(TxStreamerHandle*);
        typedef Error (*TxStreamerFree)(TxStreamerHandle*);

        typedef void* TxMetadataHandle;
        typedef Error (*TxMetadataMake)     (TxMetadataHandle*, bool, time_t, double, bool, bool);
        typedef Error (*TxMetadataFree)     (TxMetadataHandle*);
        typedef Error (*TxMetadataLastError)(TxMetadataHandle, char*, size_t);

        typedef void* SubdevSpecHandle;
        typedef Error (*SubdevSpecMake)(SubdevSpecHandle*, const char*);
        typedef Error (*SubdevSpecFree)(SubdevSpecHandle*);

        typedef void* StringVectorHandle;
        typedef Error (*StringVectorMake)(StringVectorHandle*);
        typedef Error (*StringVectorFree)(StringVectorHandle*);
        typedef Error (*StringVectorSize)(StringVectorHandle, size_t*);
        typedef Error (*StringVectorAt)  (StringVectorHandle, size_t, char*, size_t);
        typedef Error (*Find)            (const char*, StringVectorHandle*);

        // getting and setting parameters, should work for rx and tx if not specified different
        typedef Error (*GetNumRxChannels)(USRPHandle, size_t*);
        typedef Error (*GetNumTxChannels)(USRPHandle, size_t*);

        typedef Error (*SetSampleRate)(USRPHandle, double, size_t);
        typedef Error (*GetSampleRate)(USRPHandle, size_t, double*);

        typedef Error (*SetGain)(USRPHandle, double, size_t, const char*);
        typedef Error (*GetGain)(USRPHandle, size_t, const char*, double*);
        typedef Error (*GetGainElementNames)(USRPHandle, size_t, StringVectorHandle*);

        typedef Error (*SetFrequency)(USRPHandle, TuneRequest*, size_t, TuneResult*);
        typedef Error (*GetFrequency)(USRPHandle, size_t, double*);

        typedef Error (*SetBandwidth)(USRPHandle, double, size_t);
        typedef Error (*GetBandwidth)(USRPHandle, size_t, double*);

        typedef Error (*SetAntenna) (USRPHandle, const char*, size_t);
        typedef Error (*GetAntenna) (USRPHandle, size_t, char*, size_t);
        typedef Error (*GetAntennas)(USRPHandle, size_t, StringVectorHandle*);

        typedef Error (*SetSubdevSpec)(USRPHandle, SubdevSpecHandle, size_t);

        typedef Error (*SetSource)(USRPHandle, const char*, size_t);

        typedef Error (*SetTimeUnknownPPS)(USRPHandle, time_t, double);
        typedef Error (*SetTimeNow)       (USRPHandle, time_t, double, size_t);

        typedef Error (*GetRxStream)(USRPHandle, StreamArgs*, RxStreamerHandle);
        typedef Error (*GetTxStream)(USRPHandle, StreamArgs*, TxStreamerHandle);

        typedef Error (*GetRxStreamMaxNumSamples)(RxStreamerHandle, size_t*);
        typedef Error (*GetTxStreamMaxNumSamples)(TxStreamerHandle, size_t*);

        typedef Error (*RxStreamerIssueStreamCmd)(RxStreamerHandle, StreamCmd*);

        typedef Error (*RxStreamerReceive)(RxStreamerHandle, BuffsPtr, size_t, RxMetadataHandle*, double, bool, size_t*);
        typedef Error (*TxStreamerSend)   (TxStreamerHandle, BuffsPtr, size_t, TxMetadataHandle*, double, size_t*);

        typedef Error (*GetRxMetadataErrorCode)(RxMetadataHandle, RxMetadataError*);

        struct UHDSetter
        {
            UHDSetter();
            UHDSetter (SetGain      fptr, USRPHandle usrpHandle, double gain,              size_t channel, const char* gainElement);
            UHDSetter (SetFrequency fptr, USRPHandle usrpHandle, TuneRequest* tuneRequest, size_t channel, TuneResult* tuneResult);
            UHDSetter (SetBandwidth fptr, USRPHandle usrpHandle, double bandwidth,         size_t channel);
            UHDSetter (SetAntenna   fptr, USRPHandle usrpHandle, const char* antennaName,  size_t channel);

            int8_t numArguments;
            bool containsDouble;

            // a string buffer used either by SetGain or SetAntenna to store the string. In those cases the unions
            // asCharPtr representation will point to this buffer
            static const int stringBufferSize = 14; // an 8 bit int a bool and a 14 byte string make up a nicely packed struct
            char stringBuffer[stringBufferSize];

            void* functionPointer;

            USRPHandle firstArg;

            union SecondArg
            {
                double       asDouble;
                TuneRequest* asTuneRequestPtr;
                char*        asCharPtr;
            } secondArg;

            size_t thirdArg;

            union FourthArg
            {
                TuneResult* asTuneResultPtr;
                char*       asCharPtr;
            } fourthArg;

            // The C ABI calling conventions for X86 place a floating point value on a different stack (st0)
            // This is why a function with a double in its signature needs different handling
            typedef int (*ThreeArgFunctionSecondArgIsPtr)   (USRPHandle, SecondArg, size_t);
            typedef int (*ThreeArgFunctionSecondArgIsDouble)(USRPHandle, double,    size_t);
            typedef int (*FourArgFunctionSecondArgIsPtr)    (USRPHandle, SecondArg, size_t, FourthArg);
            typedef int (*FourArgFunctionSecondArgIsDouble) (USRPHandle, double,    size_t, FourthArg);

            int invoke() const;

            void* getErrorContext();
        };

        /** Automatically frees the handles if they were created during runtime */
        ~UHDr();

        /**
         * Tries to load the UHD library at runtime. A nullptr is returned if this will fail, see the result created
         * for detailled information. You should wrap this into a std::uniqe_ptr for safe lifetime management. It will close
         * the library opened and clear all data structures allocated through the library in the destructor.
         *
         * @param library A platform dependent path or name to access the library
         * @param result  An String containing information on the success of the loading operation.
         * @return        A pointer to the UHDr instance, holding all function pointers to the loaded library or nullptr in
         *                case of a failure.
         */
        static UHDr::Ptr load (const juce::String library, juce::String &result);

        /**
         * Searches for all connected devices. See uhd::device::find() for more details.
         */
        // todo: pass the error as parameter
        juce::Array<juce::StringPairArray> findAllDevices (const juce::String args);

        /**
         * A class that wraps all access to the usrp functions in an object-oriented way. It can only be created through
         * UHRr::makeUSRP. Keep in mind that one USRP object might handle multiple hardware units.
         */
        class USRP : public ReferenceCountedObject, public RealtimeSetterThreadWithFIFO<UHDSetter, Error::errorNone, Error::realtimeCallFIFO>
        {
            friend class UHDr;
        public:

            typedef juce::ReferenceCountedObjectPtr<USRP> Ptr;

            ~USRP();

            /** Sets the sample rate for a specific rx channel*/
            juce::Result setRxSampleRate (double newSampleRate, int channelIdx);

            /** Sets the sample rate for a specific tx channel*/
            juce::Result setTxSampleRate (double newSampleRate, int channelIdx);

            /**
             * Returns the sample rate for a specific rx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getRxSampleRate (int channelIdx, Error &error);

            /**
             * Returns an array filled the sample rates for all rx channels. In case of an error, an empty array will
             * be returned. Check the error code passed for further details.
             */
            juce::Array<double> getRxSampleRates (Error &error);

            /**
             * Returns the sample rate for a specific tx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getTxSampleRate (int channelIdx, Error &error);

            /**
             * Returns an array filled the sample rates for all tx channels. In case of an error, an empty array will be
             * returned. Check the error code passed for further details.
             */
            juce::Array<double> getTxSampleRates (Error &error);

            /** Sets the gain for a specific rx channel*/
            Error setRxGain (double newGain, int channelIdx, const char* gainElement);

            /** Sets the sample rate for a specific tx*/
            Error setTxGain (double newGain, int channelIdx, const char* gainElement);

            /**
             * Returns the sample rate for a specific rx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             * @see getValidRxGainElements
             */
            double getRxGain (int channelIdx, Error &error, const char* gainElement);

            /**
             * Returns a string array filled with all valid gainElement strings that might be passed to getRxGain for
             * this channel
             */
            juce::StringArray getValidRxGainElements (int channelIdx);

            /**
             * Returns an array filled the sample rates for all rx channels. In case of an error, an emptry array will be
             * returned. Check the error code passed for further details.
             */
            juce::Array<double> getRxGains (Error &error);

            /**
             * Returns the sample rate for a specific tx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             * @see getValidTxGainElemens
             */
            double getTxGain (int channelIdx, Error &error, const char* gainElement);

            /**
             * Returns a string array filled with all valid gainElement strings that might be passed to getTxGain for
             * this channel
             */
            juce::StringArray getValidTxGainElements (int channelIdx);

            /**
             * Returns an array filled the sample rates for all tx channels. In case of an error, an empty array will be
             * returned. Check the error code passed for further details.
             */
            juce::Array<double> getTxGains (Error &error);

            /**
             * Sets the frequency for a specific rx channel. Not all TuneRequests can be handled exactly by the
             * hardware, so check the tuneResult for details on how the request was handled.
             */
            Error setRxFrequency (TuneRequest &tuneRequest, TuneResult& tuneResults, int channelIdx);

            /**
            * Sets the frequency for a specific tx channel. Not all TuneRequests can be handled exactly by the
            * hardware, so check the tuneResult for details on how the request was handled.
            */
            Error setTxFrequency (TuneRequest &tuneRequest, TuneResult& tuneResult, int channelIdx);

            /**
             * Returns the rx frequency for a specific rx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getRxFrequency (int channelIdx, Error &error);

            /**
             * Returns an array filled the rx frequencies for all rx channels. In case of an error, an empty array will
             * be returned. Check the error code passed for further details.
             */
            juce::Array<double> getRxFrequencies (Error &error);

            /**
             * Returns the tx frequency for a specific tx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getTxFrequency (int channelIdx, Error &error);

            /**
             * Returns an array filled the tx frequencies for all tx channels. In case of an error, an empty array will
             * be returned. Check the error code passed for further details.
             */
            juce::Array<double> getTxFrequencies (Error &error);

            /** Sets the analog frontend bandwidth for a specific rx channel. Most devices have a fixed bandwidth*/
            Error setRxBandwidth (double newBandwidth, int channelIdx);

            /** Sets the analog frontend bandwidth for a specific tx channel. Most devices have a fixed bandwidth*/
            Error setTxBandwidth (double newBandwidth, int channelIdx);

            /**
             * Returns the bandwidth for a specific rx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getRxBandwidth (int channelIdx, Error &error);

            /**
             * Returns an array filled the bandwidths for all rx channels. In case of an error, an empty array will be
             * returned. Check the error code passed for further details.
             */
            juce::Array<double> getRxBandwidths (Error &error);

            /**
             * Returns the bandwidth for a specific tx channel. In case of any error -1 will be returned. Check the
             * error code passed for further details.
             */
            double getTxBandwidth (int channelIdx, Error &error);

            /**
             * Returns an array filled the bandwidths for all tx channels. In case of an error, an empty array will be
             * returned. Check the error code passed for further details.
             */
            juce::Array<double> getTxBandwidths (Error &error);

            /**
             * Sets the antenna port that is used as the physical input for this channel. Note: Uses a lightweight char
             * pointer for the string for faster execution from the realtime thread.
             */
            Error setRxAntenna (const char* antennaPort, int channelIdx);

            /**
             * Sets the antenna port that is used as the physical output for this channel. Note: Uses a lightweight char
             * pointer for the string for faster execution from the realtime thread.
             */
            Error setTxAntenna (const char* antennaPort, int channelIdx);

            juce::Result setRxSubdevSpec (const juce::String& subdevSpec, int mboardIdx);

            juce::Result setTxSubdevSpec (const juce::String& subdevSpec, int mboardIdx);

            /** Returns the name of the antenna port currently used for receiving with that particular channel */
            juce::String getCurrentRxAntenna (int channelIdx, Error &error);

            /** Returns an array with the names of the antenna ports currently used for receiving with each channel */
            juce::StringArray getCurrentRxAntennas (Error &error);

            /** Returns an array with the names of all possible antenna ports for a particular rx channel */
            juce::StringArray getPossibleRxAntennas (int channelIdx, Error &error);

            /** Returns the name of the antenna port currently used for sending with that particular channel */
            juce::String getCurrentTxAntenna (int channelIdx, Error &error);

            /** Returns an array with the names of the antenna ports currently used for sending with each channel */
            juce::StringArray getCurrentTxAntennas (Error &error);

            /** Returns an array with the names of all possible antenna ports for a particular tx channel */
            juce::StringArray getPossibleTxAntennas (int channelIdx, Error &error);

            /** Sets the clock source for a particular motherboard */
            juce::Result setClockSource (const juce::String clockSource, int mboardIdx);

            /** Sets the time source for a particular motherboard */
            juce::Result setTimeSource (const juce::String timeSource, int mboardIdx);

            /** Sets the time for all motherboards after the next PPS pulse received */
            juce::Result setTimeUnknownPPS (time_t fullSecs, double fracSecs);

            /**
             * Sets the time for a particular mboard. This is needed for an mimo configuration or single channel setups.
             * Don't try to set the time source to "internal" for mimo setups and call setTimeUnknownPPS afterwards. Instead,
             * just configure the time source of the slave to "mimo" and call this function on the master device afterwards.
             */
            juce::Result setTimeNow (time_t fullSecs, double fracSecs, int mboardIdx);

            /** Returns the number of input channels currently available for this USRP */
            const int getNumInputChannels();

            /** Returns the number of output channels currently available for this USRP */
            const int getNumOutputChannels();

            /** Returns the number of physical motherboards managed by this USRP instance */
            const int getNumMboards();

            /**
             * This class wraps the rxStreamer and will be used for actual Rx work. It can only be created through
             * USRP::makeRxStream.
             */
            class RxStream {
                friend class USRP;
            public:

                ~RxStream();

                /**
                 * Returns the number of channels that are used by this streamer (This determines number of buffers to pass
                 * to receive)
                 */
                const int getNumActiveChannels();

                /** Returns the maximum number of samples that might be requested per receive call. */
                const int getMaxNumSamplesPerBlock();

                /** Call this to get the hardware started for streaming! */
                juce::Result issueStreamCmd (StreamCmd &streamCmd);

                /**
                 * This will be called for each sample block to be received. It will block until a block was received or
                 * some error occured. Check the rxMetadataError for information on success. Note that the number of samples
                 * received may be equal or less the number of samples requested for that block. Make sure that the buffer
                 * is big enough as there will be no error checking
                 * @param buffsPtr          Pointer to the buffer that should hold the new samples
                 * @param numSamples        Number of samples per channel requested for the next buffer
                 * @param error             This will be filled with an error code with each call
                 * @param onePacket         Set this to true if a single packet should be received, set it to false to
                 *                          receive a continous stream of packets
                 * @param timeoutInSeconds  A timeout to wait for a packet
                 * @return                  The number of samples per channel that were received
                 */
                int receive (BuffsPtr buffsPtr, int numSamples, Error &error, bool onePacket = false, double timeoutInSeconds = 1.0);

                /**
                 * Returns the last RxMetadataError that occured. The Error handle passed as reference will help spotting
                 * any error that might occur when calling this function.
                 */
                RxMetadataError getLastRxMetadataError (Error &error);

            private:
                RxStream (UHDr::Ptr uhdr, USRPHandle &usrpHandle, StreamArgs &streamArgs, Error &error);

                UHDr::Ptr uhd;
                RxStreamerHandle rxStreamerHandle;
                RxMetadataHandle rxMetadataHandle;
                int numActiveChannels;
                size_t maxNumSamples;

                // Masking the UHDr::errorDescription to be able to add the usrpHandle's lastError string to the error description
                std::function<juce::String(Error)> errorDescription;

                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RxStream)
            };


            /** Creates an RxStream Object to access the rx stream of the USRP */
            RxStream *makeRxStream (StreamArgs &streamArgs, Error &error);

            class TxStream {
                friend class USRP;
            public:

                ~TxStream();

                /**
                 * Returns the number of channels that are used by this streamer (This determines the buffer size
                 * for Receive operations)
                 */
                const int getNumActiveChannels();

                /** Returns the maximum number of samples that might be requested per receive call. */
                const int getMaxNumSamplesPerBlock();

                /**
                 * This will be called for each sample block to be sent. It will block until a block was sent or
                 * some error occured. Check the txMetadataError for information on success. Note that the number of samples
                 * sent may be equal or less the number of samples requested for that block
                 * @param buffsPtr          Pointer to the buffer that should hold the new samples
                 * @param numSamples        Number of samples per channel available in the buffer passed
                 * @param error             This will be filled with an error code with each call
                 * @param timeoutInSeconds  A timeout to wait for a packet
                 * @return                  The number of samples per channel that were sent
                 */
                // todo: check metadata for start/end of burst flag effects
                int send (BuffsPtr buffsPtr, int numSamples, Error &error, double timeoutInSeconds = 1.0);

                Error sendEndOfBurst();

                juce::String getLastError();

            private:
                TxStream (UHDr::Ptr uhdr, USRPHandle &usrpHandle, StreamArgs &streamArgs, Error &error);

                UHDr::Ptr uhd;
                TxStreamerHandle txStreamerHandle;
                TxMetadataHandle txMetadataStartOfBurst, txMetadataContinous, txMetadataEndOfBurst;
                TxMetadataHandle txMetadataHandle;
                int numActiveChannels;
                size_t maxNumSamples;

                // Masking the UHDr::errorDescription to be able to add the usrpHandle's lastError string to the error description
                std::function<juce::String(Error)> errorDescription;

                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TxStream)
            };

            TxStream* makeTxStream (StreamArgs& streamArgs, Error& error);


        private:
            // Constructs a USRP instance. Check the error flags to determine if it's valid.
            USRP (UHDr* uhdr, const char* args, Error &error);

            UHDr::Ptr uhd;

            USRPHandle usrpHandle;
            int numInputChannels;
            int numOutputChannels;
            int numMboards;

            // Masking the UHDr::errorDescription to be able to add the usrpHandle's lastError string to the error description
            std::function<juce::String(Error)> errorDescription;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (USRP)
        };

        /**
         * Tries to create a new USRP instance from the arguments passed. In case of success, a pointer to the created
         * object will be returned, in case of any error a nullptr will be returned. Check the error code for possible
         * reasons in this case.
         *
         * The args passed would normally be something like the IP adresses, e.g.
         * args["addr0"] = "192.168.20.1"; args["addr1"] = "192.168.20.2";
         */
        USRP::Ptr makeUSRP (juce::StringPairArray args, Error &error);

    private:

        // a helper class that takes care of closing the library and generating an automated error message containing
        // the function name that couldn't be loaded
        class LoadingError {
        public:
            LoadingError (juce::String &functionName, juce::String &result) :
                    fnName (functionName),
                    res (result) {};

            // this is called if loading the most recent function failed.
            UHDr *lastFunction() {
                res = "Error loading function " + fnName;
                return nullptr;
            }

        private:
            juce::String &fnName;
            juce::String &res;
        };

        // all loaded function pointers
        USRPMake                 usrpMake;
        USRPFree                 usrpFree;
        RxStreamerMake           rxStreamerMake;
        RxStreamerFree           rxStreamerFree;
        RxMetadataMake           rxMetadataMake;
        RxMetadataFree           rxMetadataFree;
        TxStreamerMake           txStreamerMake;
        TxStreamerFree           txStreamerFree;
        TxMetadataMake           txMetadataMake;
        TxMetadataFree           txMetadataFree;
        TxMetadataLastError      txMetadataLastError;
        SubdevSpecMake           subdevSpecMake;
        SubdevSpecFree           subdevSpecFree;
        StringVectorMake         stringVectorMake;
        StringVectorFree         stringVectorFree;
        StringVectorSize         stringVectorSize;
        Find                     find;
        StringVectorAt           stringVectorAt;
        GetNumRxChannels         getNumRxChannels;
        GetNumTxChannels         getNumTxChannels;
        SetSampleRate            setRxSampleRate;
        GetSampleRate            getRxSampleRate;
        SetSampleRate            setTxSampleRate;
        GetSampleRate            getTxSampleRate;
        SetGain                  setRxGain;
        GetGain                  getRxGain;
        SetGain                  setTxGain;
        GetGain                  getTxGain;
        GetGainElementNames      getRxGainElementNames;
        GetGainElementNames      getTxGainElementNames;
        SetFrequency             setRxFrequency;
        GetFrequency             getRxFrequency;
        SetFrequency             setTxFrequency;
        GetFrequency             getTxFrequency;
        SetBandwidth             setRxBandwidth;
        GetBandwidth             getRxBandwidth;
        SetBandwidth             setTxBandwidth;
        GetBandwidth             getTxBandwidth;
        SetAntenna               setRxAntenna;
        GetAntenna               getRxAntenna;
        GetAntennas              getRxAntennas;
        SetAntenna               setTxAntenna;
        GetAntenna               getTxAntenna;
        GetAntennas              getTxAntennas;
        SetSubdevSpec            setRxSubdevSpec;
        SetSubdevSpec            setTxSubdevSpec;
        SetSource                setClockSource;
        SetSource                setTimeSource;
        SetTimeUnknownPPS        setTimeUnknownPPS;
        SetTimeNow               setTimeNow;
        GetRxStream              getRxStream;
        GetTxStream              getTxStream;
        GetRxStreamMaxNumSamples getRxStreamMaxNumSamples;
        GetTxStreamMaxNumSamples getTxStreamMaxNumSamples;
        RxStreamerIssueStreamCmd rxStreamerIssueStreamCmd;
        RxStreamerReceive        rxStreamerReceive;
        TxStreamerSend           txStreamerSend;
        GetRxMetadataErrorCode   getRxMetadataErrorCode;

        juce::DynamicLibrary uhdLib;

        // The constructor is private to make sure that it's only created by a valid opened library through UHDr::load
        UHDr () {}

        // converts an uhd string vector to a juce::StringArray and frees the string vector afterwards. Returns an empty
        // array in case of any error
        juce::StringArray uhdStringVectorToStringArray (StringVectorHandle stringVectorHandle);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UHDr)
    };
}