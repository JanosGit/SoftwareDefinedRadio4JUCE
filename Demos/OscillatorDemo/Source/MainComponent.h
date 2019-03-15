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
            engineConfigComponent = deviceManager.getConfigurationComponentForSelectedEngine();
            setContentNonOwned (engineConfigComponent.get(), true);
        };

        void closeButtonPressed() {delete this; };
    private:
        std::unique_ptr<juce::Component> engineConfigComponent;
    };

    SafePointer<EngineConfigWindow> engineConfigWindow;

    ntlab::SDRIODeviceManager deviceManager;
    ntlab::Oscillator oscillator;
    bool engineIsRunning = false;

    juce::ComboBox engineSelectionBox;
    juce::TextButton engineConfigButton;
    juce::TextButton startStopButton;

    juce::Slider centerFreqSlider;
    juce::Slider oscillatorFreqSlider;
    juce::Label centerFreqLabel;
    juce::Label oscillatorFreqLabel;

    static const int bandwidth = 10e6;

    // ntlab::SDRIODeviceCallback member functions =================================
    void prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock) override;
    void processRFSampleBlock (ntlab::OptionalCLSampleBufferComplexFloat& rxSamples, ntlab::OptionalCLSampleBufferComplexFloat& txSamples) override;
    void streamingHasStopped() override;
    void handleError (const juce::String& errorMessage) override;

    void setUpEngine();
    void setEngineState (bool engineHasStarted);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
