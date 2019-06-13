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

#include "SDRIODeviceCallback.h"

#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <bitset>

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
#include <juce_gui_basics/juce_gui_basics.h>
#endif

namespace ntlab
{
    class SDRIOEngineConfigurationInterface
    {
    public:
        /** Can be passed to the constructor of a ConfigurationComponent to constrain the values that the user can apply */
        class ConfigurationConstraints
        {
        public:
            enum ConstrainedValue
            {
                sampleRate,
                numRxChannels,
                numTxChannels,
                rxCenterFreq,
                txCenterFreq
            };

            static ConfigurationConstraints withFixedNumChannels (int fixedNumRxChannels, int fixedNumTxChannels)
            {
                jassert (numRxChannels >= 0);
                jassert (numTxChannels >= 0);

                ConfigurationConstraints constraints;
                constraints.setFixed (numRxChannels, fixedNumRxChannels);
                constraints.setFixed (numTxChannels, fixedNumTxChannels);

                return constraints;
            }

            static ConfigurationConstraints unconstrained() {return ConfigurationConstraints(); }

            template <typename ValueType>
            void setMin (ConstrainedValue valueToConstrain, ValueType minValue)
            {
                jassert (valueTypeAndValueMatch (minValue, valueToConstrain));

                values[valueToConstrain].setStart (static_cast<double> (minValue));
                minConstrainedValues.set (valueToConstrain);
            }

            template <typename ValueType>
            void setMax (ConstrainedValue valueToConstrain, ValueType maxValue)
            {
                jassert (valueTypeAndValueMatch (maxValue, valueToConstrain));

                values[valueToConstrain].setEnd (static_cast<double> (maxValue));
                maxConstrainedValues.set (valueToConstrain);
            }

            template <typename ValueType>
            void setFixed (ConstrainedValue valueToConstrain, ValueType fixedValue)
            {
                jassert (valueTypeAndValueMatch (fixedValue, valueToConstrain));

                values[valueToConstrain].setStart (static_cast<double> (fixedValue));
                values[valueToConstrain].setEnd   (static_cast<double> (fixedValue));
                minConstrainedValues.set (valueToConstrain);
                maxConstrainedValues.set (valueToConstrain);
            }

            template <typename ValueType>
            void setRange (ConstrainedValue valueToConstrain, juce::Range<ValueType> allowedRange)
            {
                jassert (valueTypeAndValueMatch (allowedRange.getStart(), valueToConstrain));

                values[valueToConstrain].setStart (static_cast<double> (allowedRange.getStart()));
                values[valueToConstrain].setEnd   (static_cast<double> (allowedRange.getEnd()));
                minConstrainedValues.set (valueToConstrain);
                maxConstrainedValues.set (valueToConstrain);
            }

            void setUnconstrained (ConstrainedValue valueToSetUnconstrained)
            {
                values[valueToSetUnconstrained].setStart (std::numeric_limits<double>::min());
                values[valueToSetUnconstrained].setEnd   (std::numeric_limits<double>::max());
                minConstrainedValues.reset (valueToSetUnconstrained);
                maxConstrainedValues.reset (valueToSetUnconstrained);
            }

            template <typename ValueType>
            bool isValidValue (ConstrainedValue targetValue, ValueType possibleValue)
            {
                if (hasMinValue (targetValue))
                {
                    if (static_cast<double> (possibleValue) < getMinDouble (targetValue))
                        return false;
                }
                if (hasMaxValue (targetValue))
                {
                    if (static_cast<double> (possibleValue) > getMaxDouble (targetValue))
                        return false;
                }

                return true;
            }

            template <typename ValueType>
            ValueType clipToValidValue (ConstrainedValue targetValue, ValueType valueToClip)
            {
                if (!isUnconstrained (targetValue))
                    return static_cast<ValueType> (values[targetValue].clipValue (static_cast<double> (valueToClip)));

                return valueToClip;
            }

            juce::Range<double> clipToValidRange (ConstrainedValue targetValue, juce::Range<double> rangeToClip)
            {
                if (!isUnconstrained (targetValue))
                    return values[targetValue].constrainRange (rangeToClip);

                return rangeToClip;
            }

            bool isCompletelyConstrained (ConstrainedValue valueToCheck) { return minConstrainedValues[valueToCheck]  && maxConstrainedValues[valueToCheck]; }
            bool isUnconstrained (ConstrainedValue valueToCheck)         { return !minConstrainedValues[valueToCheck] && !maxConstrainedValues[valueToCheck]; }
            bool hasMinValue (ConstrainedValue valueToCheck) { return minConstrainedValues[valueToCheck]; }
            bool hasMaxValue (ConstrainedValue valueToCheck) { return maxConstrainedValues[valueToCheck]; }
            bool hasFixedValue (ConstrainedValue valueToCheck) { return !isUnconstrained (valueToCheck) && values[valueToCheck].isEmpty(); }

            double getMinDouble (ConstrainedValue valueToGet) { jassert (hasMinValue (valueToGet)); return values[valueToGet].getStart(); }
            double getMaxDouble (ConstrainedValue valueToGet) { jassert (hasMaxValue (valueToGet)); return values[valueToGet].getEnd(); }
            juce::Range<double> getAllowedRangeDouble (ConstrainedValue valueToGet) { jassert (isCompletelyConstrained (valueToGet)); return values[valueToGet]; }

            int getMinInt (ConstrainedValue valueToGet) { return static_cast<int> (getMinDouble (valueToGet)); }
            int getMaxInt (ConstrainedValue valueToGet) { return static_cast<int> (getMaxDouble (valueToGet)); }
            juce::Range<int> getAllowedRangeInt (ConstrainedValue valueToGet)
            {
                jassert (isCompletelyConstrained (valueToGet));
                int start = static_cast<int> (values[valueToGet].getStart());
                int end   = static_cast<int> (values[valueToGet].getEnd());
                return {start, end};
            }

        private:
            ConfigurationConstraints()
            {
                for (auto& val : values)
                    val = juce::Range<double> (std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
            };

            static const int numValues = 5;

            std::bitset<numValues> minConstrainedValues;
            std::bitset<numValues> maxConstrainedValues;
            std::array<juce::Range<double>, numValues> values;

            template <typename ValueType>
            static constexpr bool valueTypeAndValueMatch (ValueType, ConstrainedValue constrainedValue)
            {
                if ((std::is_same<ValueType, int>::value) && ((constrainedValue == numRxChannels) || (constrainedValue == numTxChannels)))
                    return true;

                if ((std::is_same<ValueType, double>::value) && ((constrainedValue == rxCenterFreq) || (constrainedValue == txCenterFreq)))
                    return true;

                return false;
            }
        };

        virtual ~SDRIOEngineConfigurationInterface () {};

        /**
         * As an engine might handle multiple devices with heterogenous hardware capabilities this call returns a tree
         * of all available devices, sub devices, etc. depending on the actual structure of the engine.
         */
        virtual juce::ValueTree getDeviceTree() = 0;

        /** Returns the currently active config if any exists */
        virtual juce::ValueTree getActiveConfig() = 0;

        /** Applies a complete config. If this was successful it returns true, otherwise it returns false */
        virtual juce::Result setConfig (juce::ValueTree& configToSet) = 0;

    };

    /**
     * A base class for all SDRIOEngines. Note that those functions are not intended to be called directly but should
     * be invoked through the SDRIODeviceManager. Also note that you should supply a corresponding SDRIOEngineManager
     * subclass.
     *
     * @see SDRIOEngineManager
     */
    class SDRIOEngine : public SDRIOEngineConfigurationInterface
    {
    public:
        virtual ~SDRIOEngine() {};

        static const int allChannels = -1;

        virtual const int getNumRxChannels() = 0;

        virtual const int getNumTxChannels() = 0;

        /**
         * Tries to set a desired block size. If true is returned it is guaranteed that no block passed to the callback
         * is bigger than the desired block size, however smaller blocks can occur at any time.
         */
        virtual bool setDesiredBlockSize (int desiredBlockSize) = 0;

        /**
         * Tries to set the sample rate used for input and output. Multi-samplerate setups are not supported at this
         * time. Returns true if the exact sample rate could be applied, false otherwise. Check getSampleRate() in this
         * case to find out if a sample rate value near to the desired value was applied.
         */
        virtual bool setSampleRate (double newSampleRate) = 0;

        /** Returns the current sample rate used for input and output. Returns 0 if no sample rate is configured */
        virtual double getSampleRate() = 0;

        /** Returns true if the engine is configured and can start to stream */
        virtual bool isReadyToStream() = 0;

        /**
         * The engine should call SDRIODeviceCallback::prepareForStreaming first and then start to call
         * SDRIODeviceCallback::processRFSampleBlock on a regular basis
         */
        virtual bool startStreaming (SDRIODeviceCallback* callback) = 0;

        /** The engine should stop streaming and call SDRIODeviceCallback::streamingHasStopped afterwards */
        virtual void stopStreaming() = 0;

        /** Returns true if the engine is currenctly streaming */
        virtual bool isStreaming() = 0;

        /**
         * Allows you to enable and disable rx and tx, also from within the SDRIODeviceCallback::processRFSampleBlock
         * callback. This allows full or half duplex applications, depending on the hardware capabilities. Returns true
         * if the setting could be applied, false otherwise. E.g. if the hardware supports only half duplex a call to
         * enableRxTxChannels(true, true) would fail while enableRxTxChannels(true, false) or
         * enableRxTxChannels(false, true) would both be successful. Disabling both, in- and output will fail in every
         * case.
         * If you call this from withing the SDRIODeviceCallback::processRFSampleBlock callback settings will be applied
         * in the next callback.
         */
        virtual bool enableRxTx (bool enableRx, bool enableTx) = 0;

#if NTLAB_USE_CL_SAMPLE_BUFFER_COMPLEX_FOR_SDR_IO_DEVICE_CALLBACK
        virtual void setupOpenCL (cl::Context& contextToUse, cl::CommandQueue& queueToUse) = 0;
#endif
    };

    /**
     * A subclass of this is needed for every SDRIOEngine subclass. An SDRIOEngineManager can check the availabylity
     * of an SDRIOEngine on a particular system at runtime without creating a whole instance of the engine by a call
     * to isEngineAvailable. The static methods of this class help to hold all available engines managers and create
     * an engine by name.
     */
    class SDRIOEngineManager
    {
        friend class SDRIODeviceManager;
        friend class juce::ContainerDeletePolicy<SDRIOEngineManager>;
        friend class MCVFileEngineTest;
    private:

        /** Returns an array of all engines that can be created */
        static juce::StringArray getAvailableEngines();

        /** Register an SDREngineManager subclass to be able to use it to create a specific engine */
        static void registerSDREngine (SDRIOEngineManager* engineManager);

        /** Register all engines that come with this module */
        static void registerDefaultEngines();

        /**
         * Clears all Engines from the SDRIOEngineManager.
         * Although this is not really needed for a working release build, calling this at shutdown of your app will
         * prevent one or more false positive juce leak detector assertions that migh be thrown because of an
         * inverted deletion order of the static array owning the engines and the static leak detector.
         */
        static void clearAllRegisteredEngines();

        /**
         * Create an instance of the engine with this name. Get a list of all available engines through a call to
         * getAvailableEngines. Returns a nullpointer in case this engine could not be created.
         */
        static std::unique_ptr<SDRIOEngine> createEngine (const juce::String& engineName);

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
        /**
         * Creates a component that can be used to configure the engine type with this name. You need to pass a
         * configuration interface, this can be the engine itself for applications where the engine and the GUI run
         * in the same process. However, it is perfectly possible to create applications with a remote GUI that redirect
         * the configuration interface over some connection and run the DSP processing and GUI on different devices.
         * This will invoke the createEngineConfigurationComponent member function of the corresponding
         * SDRIOEngineManager for the engine type.
         */
        static std::unique_ptr<juce::Component> createEngineConfigurationComponent (const juce::String& engineName, SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints constraints);

        /**
         * Creates a component that can be used to configure the engine created by this manager instance. You need to
         * pass a configuration interface, this can be the engine itself for applications where the engine and the GUI
         * run in the same process. However, it is perfectly possible to create applications with a remote GUI that
         * redirect the configuration interface over some connection and run the DSP processing and GUI on different
         * devices.
         */
        virtual std::unique_ptr<juce::Component> createEngineConfigurationComponent (SDRIOEngineConfigurationInterface& configurationInterface, SDRIOEngineConfigurationInterface::ConfigurationConstraints& constraints) = 0;
#endif
    protected:
        virtual ~SDRIOEngineManager () {};

        virtual juce::String getEngineName() = 0;

        virtual juce::Result isEngineAvailable() = 0;

        virtual SDRIOEngine* createEngine() = 0;

    private:

        static juce::OwnedArray<SDRIOEngineManager> managers;
    };

    /**
     * A base class for all SDR engines that actually represent hardware devices and therefore are tunable
     */
    class SDRIOHardwareEngine : public SDRIOEngine
    {
    public:
        enum GainElement
        {
            /** Let the Engine chose which gain element to use */
            unspecified = -1,
            /** Fully analog gain, controlled by a PGA chip */
            analog = 0,
            /** Digital gain, set directly before the DAC / after the ADC */
            digital = 1
        };

        /**
         * Classes that need to be informed about a changed tuning of the hardware frontend can inherit from this
         * class. Just call addTuneChangeListener and the listener will be informed about changes. Note that the
         * channel value can always be SDRIOEngine::allChannels (-1) if a change applies to all channels.
         */
        class TuneChangeListener
        {
        public:
            virtual ~TuneChangeListener() {};

            virtual void rxBandwidthChanged (double newBandwidth, int channel) {};

            virtual void txBandwidthChanged (double newBandwidth, int channel) {};

            virtual void rxCenterFreqChanged (double newCenterFreq, int channel) {};

            virtual void txCenterFreqChanged (double newCenterFreq, int channel) {};
        };

        virtual ~SDRIOHardwareEngine () {};

        virtual bool setRxCenterFrequency (double newCenterFrequency, int channel = allChannels) = 0;

        /** Returns the current Rx Center Frequency or -1.0 in case of an error */
        virtual double getRxCenterFrequency (int channel) = 0;

        virtual bool setRxBandwidth (double newBandwidth, int channel = allChannels) = 0;

        virtual double getRxBandwidth (int channel) = 0;

        virtual double getRxLOFrequency (int channel = allChannels) {return 0.0;}

        virtual bool setRxGain (double newGain, GainElement gainElement = GainElement::unspecified, int channel = allChannels) = 0;

        virtual double getRxGain (int channel, GainElement gainElement = GainElement::unspecified) = 0;

        virtual bool setTxCenterFrequency (double newCenterFrequency, int channel = allChannels) = 0;

        virtual double getTxCenterFrequency (int channel) = 0;

        virtual bool setTxBandwidth (double newBandwidth, int channel = allChannels) = 0;

        virtual double getTxBandwidth (int channel) = 0;

        virtual double getTxLOFrequency (int channel = allChannels) {return 0.0;}

        virtual bool setTxGain (double newGain, GainElement gainElement = GainElement::unspecified, int channel = allChannels) = 0;

        virtual double getTxGain (int channel, GainElement gainElement = GainElement::unspecified) = 0;

        /** Adds a TuneChangeListener and calls all four change notification messages to initialize the new listener */
        void addTuneChangeListener (TuneChangeListener* newListener);

        void removeTuneChangeListener (TuneChangeListener* listenerToRemove);

    protected:

        void notifyListenersRxBandwidthChanged (double newRxBandwidth, int channel);

        void notifyListenersTxBandwidthChanged (double newTxBandwidth, int channel);

        void notifyListenersRxCenterFreqChanged (double newRxCenterFreq, int channel);

        void notifyListenersTxCenterFreqChanged (double newTxCenterFreq, int channel);

    private:
        juce::Array<TuneChangeListener*> tuneChangeListeners;
    };
}