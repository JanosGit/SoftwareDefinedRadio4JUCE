#include "ChildProcessReplacement.h"
#import <Foundation/Foundation.h>

namespace ntlab
{
    class ChildProcess::ActiveProcess
    {
    public:
        ActiveProcess (const juce::StringArray& arguments, int streamFlags, juce::Result& successLaunchingTask)
        {
            JUCE_AUTORELEASEPOOL
            {
                auto exe = arguments[0].unquoted();

                if (!juce::File::isAbsolutePath (exe))
                {
                    juce::File exeBin ("/bin/" + exe);
                    juce::File exeUsrBin ("/usr/bin/" + exe);
                    juce::File exeUsrLocalBin ("/usr/local/bin/" + exe);
                    if (exeBin.existsAsFile())
                    {
                        exe = exeBin.getFullPathName();
                    }
                    else if (exeUsrBin.existsAsFile())
                    {
                        exe = exeUsrBin.getFullPathName();
                    }
                    else if (exeUsrLocalBin.existsAsFile())
                    {
                        exe = exeUsrLocalBin.getFullPathName();
                    }
                    else
                    {
                        successLaunchingTask = juce::Result::fail ("Error finding executable " + exe + ". Try an absolute path instead");
                        return;
                    }
                }

                NSMutableArray* taskArguments = [[NSMutableArray alloc] init];
                for (int i = 1; i < arguments.size(); ++i)
                {
                    [taskArguments addObject: juceStringToNS (arguments[i])];
                }

                pipe = [[NSPipe alloc] init];
                pipeReadHandle = pipe.fileHandleForReading;

                task = [[NSTask alloc] init];
                task.arguments = taskArguments;
                if (streamFlags & ChildProcess::StreamFlags::wantStdOut)
                    task.standardOutput = pipe;
                if (streamFlags & ChildProcess::StreamFlags::wantStdErr)
                    task.standardError = pipe;
                task.terminationHandler = [this](NSTask*){ taskHasFinished.signal(); };

                [taskArguments autorelease];

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_13
                @try
                {
                    task.launchPath = juceStringToNS (exe);
                    [task launch];
                }
                @catch (NSException *e)
                {
                    successLaunchingTask = juce::Result::fail ("Exception occurred while launching Task: " + nsStringToJuce ([e reason]));
                    return;
                }

#else
                task.executableURL = createNSURLFromFile ("file://" + exe);

                NSError* error;
                BOOL success = [task launchAndReturnError: &error];
                if (!success)
                {
                    successLaunchingTask = juce::Result::fail ("An error occurred while launching Task: " + nsStringToJuce ([error localizedDescription]));
                    return;
                }
#endif
                successLaunchingTask = juce::Result::ok();
            };
        }


        ~ActiveProcess()
        {
            // this might be the case if constructor could not create the executable url
            if (task == nil)
                return;

            if (!taskHasFinished.wait (1000))
                killProcess();

            [pipeReadHandle closeFile];
        }

        bool waitForProcessToFinish (int timeoutMs)
        {
            return taskHasFinished.wait (timeoutMs);
        }

        bool isRunning()
        {
            return task.running;
        }

        int read (void* destBuffer, int numBytesToRead)
        {
            NSUInteger numBytesRead = 0;

            JUCE_AUTORELEASEPOOL
            {
                NSData *data = [pipeReadHandle readDataOfLength:static_cast<NSUInteger> (numBytesToRead)];
                numBytesRead = data.length;
                std::memcpy (destBuffer, data.bytes, numBytesRead); // is it possible to write the data directly to the buffer?
            };

            return static_cast<int> (numBytesRead);
        }

        juce::String readAllProcessOutput()
        {
            juce::String processOutput;

            JUCE_AUTORELEASEPOOL
            {
                [task waitUntilExit];
                NSData *data = [pipeReadHandle readDataToEndOfFile];
                processOutput = juce::String::fromUTF8 (static_cast<const char*> (data.bytes), static_cast<int> (data.length));
            };

            return processOutput;
        }

        bool killProcess()
        {
            [task terminate];
            return !task.running;
        }

        juce::uint32 getExitCode()
        {
            return static_cast<juce::uint32> (task.terminationStatus);
        }

        NSTask*       task = nil;
        NSFileHandle* pipeReadHandle = nil;
        NSPipe*       pipe = nil;
        juce::WaitableEvent taskHasFinished;

        // Three handy converter functions copied from juce_osx_ObjCHelpers.h
        // todo: try to include the juce_osx_ObjCHelpers instead
        static inline juce::String nsStringToJuce (NSString* s)
        {
            return juce::CharPointer_UTF8 ([s UTF8String]);
        }

        static inline NSString* juceStringToNS (const juce::String& s)
        {
            return [NSString stringWithUTF8String: s.toUTF8()];
        }

        static inline NSURL* createNSURLFromFile (const juce::String& f)
        {
            return [NSURL fileURLWithPath: juceStringToNS (f)];
        }
    };

    ChildProcess::ChildProcess ()
    {
    }

    ChildProcess::~ChildProcess ()
    {
    }

    bool ChildProcess::start (const juce::String& command, int streamFlags)
    {
        auto arguments = juce::StringArray::fromTokens (command, true);
        return start (arguments, streamFlags);
    }

    bool ChildProcess::start (const juce::StringArray& arguments, int streamFlags)
    {
        if (arguments.size() == 0)
            return false;

        juce::Result launchingTask = juce::Result::ok();
        activeProcess.reset (new ActiveProcess (arguments, streamFlags, launchingTask));
        if (launchingTask.failed())
        {
            activeProcess.reset();
            DBG ("Could not start task. " + launchingTask.getErrorMessage());
            return false;
        }

        return true;
    }

    juce::String ChildProcess::readAllProcessOutput ()
    {
        if (activeProcess == nullptr)
            return juce::String();

        return activeProcess->readAllProcessOutput();
    }

    bool ChildProcess::waitForProcessToFinish (int timeoutMs) const
    {
        if (activeProcess == nullptr)
            return false;

        return activeProcess->waitForProcessToFinish (timeoutMs);
    }

    bool ChildProcess::isRunning() const
    {
        return activeProcess != nullptr && activeProcess->isRunning();
    }

    int ChildProcess::readProcessOutput (void* dest, int numBytes)
    {
        return activeProcess != nullptr ? activeProcess->read (dest, numBytes) : 0;
    }

    bool ChildProcess::kill()
    {
        return activeProcess == nullptr || activeProcess->killProcess();
    }

    uint32 ChildProcess::getExitCode() const
    {
        return activeProcess != nullptr ? activeProcess->getExitCode() : 0;
    }
}

#ifdef NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS
#include <thread>

class ChildProcessTests : public juce::UnitTest
{
public:
    ChildProcessTests() : juce::UnitTest ("ChildProcess tests") {};

    void runTest() override
    {
        // this test tries to trigger a bug encountered with the original child process implementation
        beginTest ("Call ChildProcess with invalid command");

        ntlab::ChildProcess childProcess1;
        expect (!childProcess1.start ("invalid_command"));
        auto processOutput1 = childProcess1.readAllProcessOutput();

        expect (!processOutput1.contains ("juce_LeakedObjectDetector"), "Leak detector prints in processOutput");

        beginTest ("Start ChildProcess on other thread");

#if JUCE_WINDOWS
        auto createAndStartProcess = [this]() {childProcess2.reset (new ntlab::ChildProcess); childProcess2->start ("help"); };
#else
        auto createAndStartProcess = [this]() {childProcess2.reset (new ntlab::ChildProcess); childProcess2->start ("ls /dev/"); };
#endif
        std::thread childProcess2CreationThread (createAndStartProcess);
        childProcess2CreationThread.join();

        auto processOutput2 = childProcess2->readAllProcessOutput();
        expect (processOutput2.isNotEmpty(), "Empty process output string returned");

        // read it again, this time it should be empty
        processOutput2 = childProcess2->readAllProcessOutput();
        expect (processOutput2.isEmpty());
    }

private:
    std::unique_ptr<ntlab::ChildProcess> childProcess2;
};

static ChildProcessTests childProcessTests;

#endif // NTLAB_SOFTWARE_DEFINED_RADIO_UNIT_TESTS