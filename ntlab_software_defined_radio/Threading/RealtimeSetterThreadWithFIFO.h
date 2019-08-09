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
#include <array>

/**
 * A templated class that contains a FIFO for some generic RealtimeCapableSetter structs and a thread that
 * invokes them. This class is designed to be able to execute calls to e.g. SDR hardware APIs with unpredictable
 * return time from within a realtime callback. Therefore you need to hand the thread ID of the realtime callback thread
 * to the instance of this class so that the call method can figure out if the call came from the realtime thread. In
 * this case it will push the setter into the FIFO queue to be executed as soon as possible and returns immediately.
 * In all other cases the call will directly be invoked on the calling thread.
 *
 * The RealtimeCapableSetter struct needs to be trivially copyable and needs at least two public member functions:
 * - int invoke() const;      --> invokes the function and returns an error code
 * - void* getErrorContext(); --> supplies some additional information for error handling. It's value will be handed
 *                                to the errorFromSetterThread lambda callback. Might also just return a nullptr
 */
template <typename RealtimeCapableSetter, int noErrorValue, int pushToFIFOError, int fifoSize = 32>
class RealtimeSetterThreadWithFIFO : private juce::AbstractFifo, private juce::Thread
{
public:
    static_assert (std::is_trivially_copyable<RealtimeCapableSetter>::value, "RealtimeCapableSetter must be trivially copyable");

    using NoneRealtimeCriticalLambda = std::function<int()>;

    RealtimeSetterThreadWithFIFO() : juce::AbstractFifo (fifoSize), juce::Thread ("Realtime setter thread")
    {
        startThread (8);
    };

    ~RealtimeSetterThreadWithFIFO()
    {
        stopThread (2000);
    }

    /**
     * Set the thread ID of the realtime thread so that calls from this thread will be pushed to the FIFO for later
     * execution
     */
    void setRealtimeThreadID (juce::Thread::ThreadID newRealtimeThreadID) {realtimeThreadID = newRealtimeThreadID; }

    /**
     * Invokes a RealtimeCapableSetter directly and returns it return value if this call is not coming from the real
     * time thread. If it comes from the realtime thread it will be pushed to a queue and will be executed on a
     * separate thread owned by this class. In this case, the return value will always be noErrorValue.
     */
    int call (const RealtimeCapableSetter& setter)
    {
        if (realtimeThreadID == juce::Thread::getCurrentThreadId())
        {
            auto writeHandle = write (1);

            if (writeHandle.blockSize1 == 1)
                setterQueue[writeHandle.startIndex1] = setter;
            else if (writeHandle.blockSize2 == 1)
                setterQueue[writeHandle.startIndex2] = setter;
            else
                return pushToFIFOError;

            notify();

            return noErrorValue;
        }

        return setter.invoke();
    }

    /**
     * This can be used to handle a non-realtime critical task on the setter thread. Calling this function is not
     * realtime-safe. It will invoke the lambda directly and return its return value if the call is not coming from the
     * real time thread. If it comes from the realtime thread it will be pushed to a queue and will be executed on a
     * separate thread owned by this class. In this case, the return value will always be noErrorValue.
     */
    int callLambda (NoneRealtimeCriticalLambda&& lambdaToCall)
    {
        if (realtimeThreadID == getCurrentThreadId())
        {
            lambdas.push_back (lambdaToCall);
            notify();
            return noErrorValue;
        }

        return lambdaToCall();
    }

    /**
     * Will be invoked if a RealtimeCapableSetter was executed on the internal thread and threw an error. The context
     * pointer will be the return value of RealtimeCapableSetter::getErrorContext
     */
    std::function<void(int, void*)> errorFromSetterThread = [] (int returnValue, void* context) {};

private:
    std::array<RealtimeCapableSetter, fifoSize> setterQueue;
    juce::Thread::ThreadID realtimeThreadID = nullptr;

    std::vector<NoneRealtimeCriticalLambda> lambdas;

    void run() override
    {
        while (!threadShouldExit())
        {
            auto numReady = getNumReady();
            if (numReady)
            {
                auto readHandle = read (numReady);

                for (auto i = 0; i != readHandle.blockSize1; ++i)
                {
                    int error = setterQueue[readHandle.startIndex1 + i].invoke();
                    if (error != noErrorValue)
                        errorFromSetterThread (error, setterQueue[readHandle.startIndex1 + i].getErrorContext());
                }
                for (auto i = 0; i != readHandle.blockSize2; ++i)
                {
                    int error = setterQueue[readHandle.startIndex2 + i].invoke();
                    if (error != noErrorValue)
                        errorFromSetterThread (error, setterQueue[readHandle.startIndex2 + i].getErrorContext());
                }
            }

            for (auto& lambda : lambdas)
                lambda();

            lambdas.clear();

            wait (-1);
        }
    }

};