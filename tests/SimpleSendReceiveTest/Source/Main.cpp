/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include "../JuceLibraryCode/JuceHeader.h"

// This class is used later to handle all realtime sample processing. Creating your own class inheriting from
// ntlab::SDRIODeviceCallback allows you to execute your custom DSP code. This class must override those four
// pure virtual member functions you see here. Our callback just sends out a sine wave and prints the number of
// samples received per block to stdout
class Callback : public ntlab::SDRIODeviceCallback
{
public:

    // Prepare for streaming is called once before the first call to processRFSampleBlock occurs. Normally this is
    // the place where you set up ressources as DSP processing blocks or sample buffers.
    void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock) override
    {
        std::cout << "prepareForStreaming called\n. The expected max number of samples per block is " << maxNumSamplesPerBlock << "\n";

        // store sample rate
        currentSampleRate = sampleRate;

        // do some pre calculations for the oscillator
        auto cyclesPerSample = oscillatorFrequency / currentSampleRate;
        angleDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    }

    // This function will be called repeatedly by the engine to let you process the next block of samples
    void processRFSampleBlock (ntlab::OptionalCLSampleBufferComplexFloat& rxSamples, ntlab::OptionalCLSampleBufferComplexFloat& txSamples) override
    {
        const int numRxSamples = rxSamples.getNumSamples();
        const int numTxSamples = txSamples.getNumSamples();

        // Note that this is done for demonstration tasks only. You normally never ever should call system calls like
        // std::cout as well as file access and memory allocation from this function as those functions might be blocking
        // for an unpredictable amount of time which can cause dropouts and buffer underruns
        std::cout << "Received " << numRxSamples << " samples\n";

        auto* chan0TxBuffer = txSamples.getWritePointer (0);
        auto* chan1TxBuffer = txSamples.getWritePointer (1);

        for (auto sampleIdx = 0; sampleIdx < numTxSamples; ++sampleIdx)
        {
            // calculate the current complex valued sample
            auto currentSample = std::complex<float> (static_cast<float> (std::sin (currentAngle)),
                                                      static_cast<float> (std::cos (currentAngle)));

            // assign this sample to both buffers
            chan0TxBuffer[sampleIdx] = currentSample;
            chan1TxBuffer[sampleIdx] = currentSample;

            // advance the angle for the next sample
            currentAngle += angleDelta;
        }
    }

    // Streaming has stopped is guranteed to be called after the last sample block has been processed. This is the right
    // place to free ressources that were allocated in prepareForStreaming
    void streamingHasStopped() override
    {
        std::cout << "streamingHasStopped called\n";
    }

    // In case any error should occur while streaming is in progress, this function will be invoked
    void handleError (const juce::String& errorMessage) override
    {
        std::cerr << "Streaming error: " << errorMessage;
    }

private:

    double currentSampleRate = 0.0, currentAngle = 0.0, angleDelta = 0.0;
    const double oscillatorFrequency = 1.8e6;
};




int main (int argc, char* argv[])
{
    // This will query the system for all engines available and register the ones that are available.
    // Note: If the UHD Engine is available this means that the UHD Library could be successfully loaded on this
    // System. However, this does not necessarily mean that there is any USRP connected to the computer
    ntlab::SDRIOEngineManager::registerDefaultEngines();
    auto availableEngines = ntlab::SDRIOEngineManager::getAvailableEngines();
    for (auto& engine : availableEngines)
        std::cout << engine << std::endl;

    // Exit if the UHD Engine is not available on this machine
    if (!availableEngines.contains ("UHD Engine"))
    {
        std::cerr << "UHD Engine not found" << std::endl;
        return 1;
    }

    // Create the UHD Engine through the manager and create a UHDEngine reference from it. Side not: You might want
    // to use auto here, however the full type names are used here to clarify what's going on. As the unique_ptr manages
    // the lifetime of the engine instance it is important to keep in mind that although the specific UHDEngine instance
    // will be accessed by an UHDEngine reference, the engine pointers lifetime must not be shorter than the uhdEngine
    // reference.
    std::unique_ptr<ntlab::SDRIOEngine>  enginePtr = ntlab::SDRIOEngineManager::createEngine ("UHD Engine");
    ntlab::UHDEngine&                    uhdEngine = ntlab::UHDEngine::castToUHDEngineRef (enginePtr);

    // This call is needed to suppress unintended false error assertions from the juce leak detector in debug builds.
    // Not needed for release builds.
    ntlab::SDRIOEngineManager::clearAllRegisteredEngines();

    // Find out which devices are available for the UHD Engine. The juce JSON::toString method creates a nicely readable
    // version of the device tree which is a juce::var with nested properties
    // std::cout << juce::JSON::toString (uhdEngine.getDeviceTree()) << std::endl;

    // A call to makeUSRP activates the desired devices that the device tree returned. Note that those hard coded
    // IP addresses are here for demonstration purposes only and should be changed to a value fitting your hardware
    // setup. Also chose a synchronization setup that suits your hardware setup.
    auto makeUSRP = uhdEngine.makeUSRP ({"192.168.20.1", "192.168.20.3"}, ntlab::UHDEngine::SynchronizationSetup::twoDevicesMIMOCableMasterSlave);
    if (makeUSRP.failed())
    {
        std::cerr << makeUSRP.getErrorMessage() << std::endl;
        return 1;
    }

    // for each input and output channel you want to use create a channel setup instance
    ntlab::UHDEngine::ChannelSetup chan0, chan1;
    chan0.mboardIdx               = 0;
    chan0.daughterboardSlot       = "A";
    chan0.frontendOnDaughterboard = "0";
    chan0.antennaPort             = "RX2";

    chan1.mboardIdx               = 1;
    chan1.daughterboardSlot       = "A";
    chan1.frontendOnDaughterboard = "0";
    chan1.antennaPort             = "RX2";

    // create an array of those channel setups and pass it to setupRxChannels. The order of the single channel setups
    // in this array defines which channel number in the sample buffers processed later and all other get/set functions
    // a certain hardware channel refers to.
    juce::Array<ntlab::UHDEngine::ChannelSetup> inputChannelSetup ({chan0, chan1});
    auto setupRxChannels = uhdEngine.setupRxChannels (inputChannelSetup);
    if (setupRxChannels.failed())
    {
        std::cerr << setupRxChannels.getErrorMessage() << std::endl;
        return 1;
    }

    // re-use the structs created above and just change the antenna port. Then construct an output channel setup array
    // from them and set up the tx channels just like the rx channels
    chan0.antennaPort = "TX/RX";
    chan1.antennaPort = "TX/RX";
    juce::Array<ntlab::UHDEngine::ChannelSetup> outputChannelSetup ({chan0, chan1});
    auto setupTxChannels = uhdEngine.setupTxChannels (outputChannelSetup);
    if (setupTxChannels.failed())
    {
        std::cerr << setupTxChannels.getErrorMessage() << std::endl;
        return 1;
    }

    // Set a sample rate for both rx and tx
    if (!uhdEngine.setSampleRate (2e6))
    {
        std::cerr << "Could not set desired sample rate" << std::endl;
        return 1;
    }

    // Set center frequency, bandwidth and gain for rx and tx
    if (!uhdEngine.setRxCenterFrequency (1.89e9, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired rx center frequency" << std::endl;
        return 1;
    }
    if (!uhdEngine.setRxBandwidth (0.25e6, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired rx bandwidth" << std::endl;
        return 1;
    }
    if (!uhdEngine.setRxGain (1.0, ntlab::SDRIOHardwareEngine::GainElement::unspecified, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired rx gain" << std::endl;
        return 1;
    }

    if (!uhdEngine.setTxCenterFrequency (1.89e9, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired tx center frequency" << std::endl;
        return 1;
    }
    if (!uhdEngine.setTxBandwidth (2.0e6, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired tx bandwidth" << std::endl;
        return 1;
    }
    if (!uhdEngine.setTxGain (1.0, ntlab::SDRIOHardwareEngine::GainElement::unspecified, ntlab::SDRIOEngine::allChannels))
    {
        std::cerr << "Could not set desired tx gain" << std::endl;
        return 1;
    }

    // Check if the engine is ready to stream
    if (!uhdEngine.isReadyToStream())
    {
        std::cerr << "Engine is not ready to stream" << std::endl;
        return 1;
    }

    // Create an instance of the Callback class defined above the main
    Callback callback;

    // Instruct the engine to start streaming with the settings applied above and use the callback instance created above
    uhdEngine.startStreaming (&callback);

    // As all streaming is executed on a seperate high-priority thread, owned by the engine. The main thread can just
    // sleep for 5 seconds letting the stream run for this time
    juce::Thread::sleep (5000);

    // After waiting for 5 seconds this instructs the engine to stop the stream
    uhdEngine.stopStreaming();

    return 0;
}
