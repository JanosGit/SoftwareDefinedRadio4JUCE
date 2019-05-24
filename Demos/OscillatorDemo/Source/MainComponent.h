/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public juce::Component, private ntlab::SDRIODeviceCallback
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    // juce::Component member functions ==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==============================================================================
    class EngineConfigWindow : public juce::DocumentWindow
    {
    public:
        EngineConfigWindow (ntlab::SDRIODeviceManager& deviceManager) : DocumentWindow ("Configure Engine", juce::Colours::black, allButtons)
        {
            auto constraints = ntlab::SDRIOEngineConfigurationInterface::ConfigurationConstraints::withFixedNumChannels (0, 1);

            constraints.setMin (ntlab::SDRIOEngineConfigurationInterface::ConfigurationConstraints::rxCenterFreq, 20e6);

            engineConfigComponent = deviceManager.getConfigurationComponentForSelectedEngine (constraints);
            setContentNonOwned (engineConfigComponent.get(), true);
        };

        void closeButtonPressed() {delete this; };
    private:
        std::unique_ptr<juce::Component> engineConfigComponent;
    };

    SafePointer<EngineConfigWindow> engineConfigWindow;

    ntlab::SDRIODeviceManager deviceManager;
    std::unique_ptr<ntlab::Oscillator> oscillator;
    bool engineIsRunning = false;

    juce::ComboBox engineSelectionBox;
    juce::TextButton engineConfigButton;
    juce::TextButton startStopButton;

    juce::Slider centerFreqSlider;
    juce::Slider oscillatorFreqSlider;
    juce::Label centerFreqLabel;
    juce::Label oscillatorFreqLabel;

    double bandwidth = 10e6;

    static const juce::File settingsFile;

#if NTLAB_USE_CL_DSP
    // CL stuff
    cl::Platform     platform;
    cl::Device       device;
    cl::Context      context;
    cl::CommandQueue queue;
    cl::Program      program;

	juce::Result setUpCL();
#endif

    // ntlab::SDRIODeviceCallback member functions =================================
    void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock) override;
    void processRFSampleBlock (ntlab::OptionalCLSampleBufferComplexFloat& rxSamples, ntlab::OptionalCLSampleBufferComplexFloat& txSamples) override;
    void streamingHasStopped() override;
    void handleError (const juce::String& errorMessage) override;

    void setUpEngine();
    void setEngineState (bool engineHasStarted);
    void setupSliderRanges (double centerFreq);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
