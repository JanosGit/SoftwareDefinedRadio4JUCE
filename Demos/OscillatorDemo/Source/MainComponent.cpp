/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent() : oscillator (1)
{
    // Initialize our device manager first
    deviceManager.addDefaultEngines();

    // Add our sub components to the MainComponent
    addAndMakeVisible (engineSelectionBox);
    addAndMakeVisible (engineConfigButton);
    addAndMakeVisible (startStopButton);
    addAndMakeVisible (centerFreqSlider);
    addAndMakeVisible (oscillatorFreqSlider);

    // Fill the combo box with all engines available
    engineSelectionBox.addItemList (deviceManager.getEngineNames(), 1);

    // Style the GUI Components
    engineConfigButton.setButtonText ("Configure Engine");
    startStopButton   .setButtonText ("Start");

    centerFreqSlider    .setSliderStyle (juce::Slider::SliderStyle::Rotary);
    oscillatorFreqSlider.setSliderStyle (juce::Slider::SliderStyle::Rotary);

    centerFreqSlider    .setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 200, 20);
    oscillatorFreqSlider.setTextBoxStyle (juce::Slider::TextEntryBoxPosition::TextBoxBelow, false, 200, 20);

    centerFreqSlider    .setTextValueSuffix ("Hz");
    oscillatorFreqSlider.setTextValueSuffix ("Hz");

    centerFreqLabel    .setText ("SDR center frequency", juce::NotificationType::dontSendNotification);
    oscillatorFreqLabel.setText ("Oscillator frequency", juce::NotificationType::dontSendNotification);

    centerFreqLabel    .setJustificationType (juce::Justification::centredBottom);
    oscillatorFreqLabel.setJustificationType (juce::Justification::centredBottom);

    centerFreqLabel    .attachToComponent (&centerFreqSlider, false);
    oscillatorFreqLabel.attachToComponent (&oscillatorFreqSlider, false);

    centerFreqSlider.setEnabled (false);

    // Assign callbacks
    engineSelectionBox.onChange = [this]()
    {
        auto selectedEngine = engineSelectionBox.getText();
        if (selectedEngine.isNotEmpty())
        {
            deviceManager.selectEngine (selectedEngine);
        }
    };

    engineConfigButton.onClick = [this]()
    {
        if (deviceManager.getSelectedEngine())
        {
            engineConfigWindow = new EngineConfigWindow (deviceManager);
            engineConfigWindow->setBounds(20, 50, 300, 500);
            engineConfigWindow->setResizable (true, false);
            engineConfigWindow->setUsingNativeTitleBar (true);
            engineConfigWindow->setVisible (true);
        }

    };

    startStopButton.onClick = [this]()
    {
        if (engineIsRunning)
        {
            deviceManager.stopStreaming();
        }
        else
        {
            if (deviceManager.getSelectedEngine())
            {
                setUpEngine();
            }
        }
    };

    oscillatorFreqSlider.onValueChange = [this]()
    {
        oscillator.setFrequencyHz (oscillatorFreqSlider.getValue(), ntlab::SDRIOEngine::allChannels);
    };

    centerFreqSlider.onValueChange = [this]()
    {
        const double newCenterFreq = centerFreqSlider.getValue();

        auto& hardwareEngine = dynamic_cast<ntlab::SDRIOHardwareEngine&> (*deviceManager.getSelectedEngine());
        hardwareEngine.setTxCenterFrequency (newCenterFreq, ntlab::SDRIOEngine::allChannels);

        const double newMaxOscFreq = newCenterFreq + bandwidth;
        const double currentOscFreq = oscillatorFreqSlider.getValue();
        oscillatorFreqSlider.setRange (newCenterFreq, newMaxOscFreq, 0);
        if (currentOscFreq < newCenterFreq)
            oscillatorFreqSlider.setValue (newCenterFreq, juce::NotificationType::sendNotification);
        if (currentOscFreq > newMaxOscFreq)
            oscillatorFreqSlider.setValue (newMaxOscFreq, juce::NotificationType::sendNotification);
    };

    const double initialCenterFreq = 1.89e9;
    centerFreqSlider.setRange (1.87e9, 1.91e9, 1000);
    centerFreqSlider.setValue (initialCenterFreq, juce::NotificationType::dontSendNotification);
    oscillatorFreqSlider.setRange (initialCenterFreq, initialCenterFreq + bandwidth, 10);
    oscillatorFreqSlider.setValue (initialCenterFreq, juce::NotificationType::sendNotification);

    setSize (600, 400);
}

MainComponent::~MainComponent()
{
    deviceManager.stopStreaming();
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto lowerArea = getLocalBounds();
    auto upperArea = lowerArea.removeFromTop (50);
    lowerArea.removeFromTop (30);
    lowerArea.removeFromBottom (10);

    auto width = getWidth();
    auto upperWidth = width / 3;
    auto lowerWidth = width / 2;

    engineSelectionBox.setBounds (upperArea.removeFromLeft  (upperWidth).reduced (5));
    startStopButton   .setBounds (upperArea.removeFromRight (upperWidth).reduced (5));
    engineConfigButton.setBounds (upperArea.reduced (5));

    centerFreqSlider    .setBounds (lowerArea.removeFromLeft  (lowerWidth));
    oscillatorFreqSlider.setBounds (lowerArea.removeFromRight (lowerWidth));
}

void MainComponent::prepareForStreaming (double sampleRate, int numActiveChannelsIn, int numActiveChannelsOut, int maxNumSamplesPerBlock)
{
    oscillator.setSampleRate (sampleRate);
    DBG ("Starting to stream with " << numActiveChannelsIn << " input channels, " << numActiveChannelsOut << " output channels, block size " << maxNumSamplesPerBlock << " samples");

    // Enable the center freq slider only if the engine is a hardware device that has a tunable center frequency
    if (dynamic_cast<ntlab::SDRIOHardwareEngine*> (deviceManager.getSelectedEngine()))
        juce::MessageManager::callAsync ([this]() { centerFreqSlider.setEnabled (true); });
}

void MainComponent::processRFSampleBlock (ntlab::OptionalCLSampleBufferComplexFloat& rxSamples, ntlab::OptionalCLSampleBufferComplexFloat& txSamples)
{
    juce::ScopedNoDenormals noDenormals;
    oscillator.fillNextSampleBuffer (txSamples);
}

void MainComponent::streamingHasStopped ()
{
    juce::MessageManager::callAsync ([this] ()
    {
        centerFreqSlider.setEnabled (false);
        setEngineState (false);
    });
}

void MainComponent::handleError (const juce::String& errorMessage)
{
    DBG (errorMessage);
}

void MainComponent::setUpEngine()
{
    if (auto uhdEngine = dynamic_cast<ntlab::UHDEngine*> (deviceManager.getSelectedEngine()))
    {
        auto makeUSRP = uhdEngine->makeUSRP ({"192.168.20.1"}, ntlab::UHDEngine::SynchronizationSetup::singleDeviceStandalone);
        if (makeUSRP.failed())
        {
            std::cerr << makeUSRP.getErrorMessage() << std::endl;
            return;
        }

        ntlab::UHDEngine::ChannelSetup channelSetup;
        channelSetup.mboardIdx               = 0;
        channelSetup.daughterboardSlot       = "A";
        channelSetup.frontendOnDaughterboard = "0";
        channelSetup.antennaPort             = "TX/RX";

        juce::Array<ntlab::UHDEngine::ChannelSetup> outputChannelSetup ({channelSetup});
        auto setupTxChannels = uhdEngine->setupTxChannels (outputChannelSetup);
        if (setupTxChannels.failed())
        {
            std::cerr << setupTxChannels.getErrorMessage() << std::endl;
            return;
        }

        if (!uhdEngine->setSampleRate (bandwidth))
        {
            std::cerr << "Could not set desired sample rate" << std::endl;
            return;
        }

        if (!uhdEngine->setTxCenterFrequency (centerFreqSlider.getValue(), ntlab::SDRIOEngine::allChannels))
        {
            std::cerr << "Could not set desired tx center frequency" << std::endl;
        }

        if (!uhdEngine->enableRxTx (false, true))
        {
            std::cerr << "Could not enable Tx" << std::endl;
            return;
        }

        uhdEngine->addTuneChangeListener (&oscillator);
    }
    else if (auto mcvEngine = dynamic_cast<ntlab::MCVFileEngine*> (deviceManager.getSelectedEngine()))
    {
        auto outFile = juce::File::getSpecialLocation (juce::File::SpecialLocationType::userDesktopDirectory).getChildFile ("OscillatorDemoOutput.mcv");
        mcvEngine->setOutFile (outFile, 1, true);
    }
    else
    {
        // unsupported engine
        jassertfalse;
        return;
    }

    deviceManager.setCallback (this);

    if (deviceManager.isReadyToStream())
    {
        setEngineState (true);
        deviceManager.startStreaming();
    }
    else
    {
        std::cerr << "Engine is not ready to stream" << std::endl;
    }
}

void MainComponent::setEngineState (bool engineHasStarted)
{
    if (engineHasStarted)
    {
        startStopButton.setButtonText ("Stop");
        engineIsRunning = true;
    }
    else
    {
        startStopButton.setButtonText ("Start");
        engineIsRunning = false;
    }
}