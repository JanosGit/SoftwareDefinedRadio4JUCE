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

#include <juce_events/juce_events.h>

namespace ntlab
{
    /**
     * A class to do simple profiling of the sample processing callback. It starts a timer on the message thread
     * that will poll the internal counter and make sure that you roughly get a measurement result afer a desired
     * number samples. The result will contain an the average time spent per sample in microsecond, a load in percent
     * based on the theoretical time available per sample at the given samplerate (note that this does not take in
     * account any io overhead) and the actual number of samples the result is based on.
     *
     * In the actual processing callback, a ScopedProcessingTimeMeasurement helps recording the processing time.
     */
    class ProcessingTimeMeasurement : private juce::Thread
    {
    public:

        /**
         * The object that helps recording the time spent in the processing callback. You cannot create one directly,
         * use startScopedMeasurement instead.
         */
        class ScopedProcessingTimeMeasurement
        {
            friend class ProcessingTimeMeasurement;
        public:
            ~ScopedProcessingTimeMeasurement()
            {
                auto delta = juce::Time::getHighResolutionTicks() - startTime;

                juce::GenericScopedLock<juce::SpinLock> l (parent.counterLock);
                parent.ticks    += delta;
                parent.numSamps += numSamples;
            }

        private:
            ProcessingTimeMeasurement& parent;
            const int numSamples;
            const juce::int64 startTime;

            ScopedProcessingTimeMeasurement (ProcessingTimeMeasurement& parentInstance, const int numSamplesThisBlock)
             : parent     (parentInstance),
               numSamples (numSamplesThisBlock),
               startTime  (juce::Time::getHighResolutionTicks()) {}
        };

        using MeasurementResultCallback = std::function<void (double microSecondsPerSample, double percentsOfProcessingTimeUsedPerSample, juce::int64 numSamplesAveraged)>;

        /**
         * Creates a ProcessingTimeMeasurement that roughly invokes the measurement result callback after the desired
         * number of samples has been processed. The default callback will print the results to the current juce::Logger,
         * you can alternatively pass in a custom callback. The callback will always be invoked from a thread owned by
         * this class.
         */
        ProcessingTimeMeasurement (int numSamplesToAverage,
                                   MeasurementResultCallback callback = [] (double us, double load, juce::int64)
                                   {
                                        juce::Logger::writeToLog ("Processing time measurement results: " + juce::String (us) + "μs per sample, " + juce::String (load) + "% load");
                                   })
         : juce::Thread ("ntlab_ProcessingTimeMeasurement_thread"),
           resultCallback    (callback),
           numSampsToAverage (numSamplesToAverage) {}

        /** Always call this before starting to stream */
        void processingStarts (double sampleRate)
        {
            availableProcessingTimePerSampleMicroSeconds = 1e6 / sampleRate;

            double timeForNumSampsToAverage  = numSampsToAverage / sampleRate;
            periodMilliseconds = static_cast<int> (timeForNumSampsToAverage * 1000);

            startThread (1);
        }

        /** Always call this afer streaming has stopped */
        void processingEnds()
        {
            stopThread (2 * periodMilliseconds);
        }

        /**
         * This creates a ScopedProcessingTimeMeasurement that will record the time spent in the callback.
         * Create one on the stack just before the desired part of the processing you want to process and let it go
         * out of scope after the desired part of the processing is over
         */
        const ScopedProcessingTimeMeasurement startScopedMeasurement (const int numSamplesThisBlock)
        {
            return ScopedProcessingTimeMeasurement (*this, numSamplesThisBlock);
        }

    private:
        juce::SpinLock            counterLock;
        juce::int64               ticks = 0, numSamps = 0;
        MeasurementResultCallback resultCallback;
        const int                 numSampsToAverage;
        int                       periodMilliseconds;
        double                    availableProcessingTimePerSampleMicroSeconds = 0;

        void run() override
        {
            juce::int64 currentTicks, currentNumSamps;

            while (!threadShouldExit())
            {
                wait (periodMilliseconds);
                {
                    juce::GenericScopedLock<juce::SpinLock> l (counterLock);

                    if (numSamps < numSampsToAverage)
                        continue;

                    currentTicks    = ticks;
                    currentNumSamps = numSamps;

                    ticks    = 0;
                    numSamps = 0;
                }

                invokeResultCallback (currentTicks, currentNumSamps);
            }

            invokeResultCallback (ticks, numSamps);

            ticks = 0;
            numSamps = 0;
        }

        void invokeResultCallback (juce::int64 currentTicks, juce::int64 currentNumSamps)
        {
            auto timeInMicroSeconds = juce::Time::highResolutionTicksToSeconds (currentTicks) * 1e6;
            auto timePerSample = timeInMicroSeconds / currentNumSamps;

            auto percentsOfProcessingTimeUsedPerSample = (timePerSample / availableProcessingTimePerSampleMicroSeconds) * 100;

            resultCallback (timePerSample, percentsOfProcessingTimeUsedPerSample, currentNumSamps);
        }
    };

    class LightweightProcessingTimeMeasurement
    {
    public:

        /**
         * The object that helps recording the time spent in the processing callback. You cannot create one directly,
         * use startScopedMeasurement instead.
         */
        class ScopedProcessingTimeMeasurement
        {
            friend class LightweightProcessingTimeMeasurement;
        public:
            ~ScopedProcessingTimeMeasurement()
            {
                auto delta = juce::Time::getHighResolutionTicks() - startTime;

                parent.ticks    += delta;
                parent.numSamps += numSamples;
            }

        private:
            LightweightProcessingTimeMeasurement& parent;
            const int numSamples;
            const juce::int64 startTime;

            ScopedProcessingTimeMeasurement (LightweightProcessingTimeMeasurement& parentInstance, const int numSamplesThisBlock)
                    : parent     (parentInstance),
                      numSamples (numSamplesThisBlock),
                      startTime  (juce::Time::getHighResolutionTicks()) {}
        };

        using MeasurementResultCallback = std::function<void (double microSecondsPerSample, double percentsOfProcessingTimeUsedPerSample, juce::int64 numSamplesAveraged)>;

        /**
         * Creates a ProcessingTimeMeasurement that roughly invokes the measurement result callback after the desired
         * number of samples has been processed. The default callback will print the results to the current juce::Logger,
         * you can alternatively pass in a custom callback. The callback will always be invoked from a thread owned by
         * this class.
         */
        LightweightProcessingTimeMeasurement (MeasurementResultCallback callback = [] (double us, double load, juce::int64)
                                              {
                                                 juce::Logger::writeToLog ("Processing time measurement results: " + juce::String (us) + "μs per sample, " + juce::String (load) + "% load");
                                              })
                : resultCallback (callback) {}

        /** Always call this before starting to stream */
        void processingStarts (double sampleRate)
        {
            availableProcessingTimePerSampleMicroSeconds = 1e6 / sampleRate;
        }

        /** Always call this afer streaming has stopped */
        void processingEnds()
        {
            invokeResultCallback (ticks, numSamps);
            ticks = 0;
            numSamps = 0;
        }

        /**
         * This creates a ScopedProcessingTimeMeasurement that will record the time spent in the callback.
         * Create one on the stack just before the desired part of the processing you want to process and let it go
         * out of scope after the desired part of the processing is over
         */
        const ScopedProcessingTimeMeasurement startScopedMeasurement (const int numSamplesThisBlock)
        {
            return ScopedProcessingTimeMeasurement (*this, numSamplesThisBlock);
        }

    private:
        juce::int64               ticks = 0, numSamps = 0;
        MeasurementResultCallback resultCallback;
        double                    availableProcessingTimePerSampleMicroSeconds = 0;

        void invokeResultCallback (juce::int64 currentTicks, juce::int64 currentNumSamps)
        {
            auto timeInMicroSeconds = juce::Time::highResolutionTicksToSeconds (currentTicks) * 1e6;
            auto timePerSample = timeInMicroSeconds / currentNumSamps;

            auto percentsOfProcessingTimeUsedPerSample = (timePerSample / availableProcessingTimePerSampleMicroSeconds) * 100;

            resultCallback (timePerSample, percentsOfProcessingTimeUsedPerSample, currentNumSamps);
        }
    };
}