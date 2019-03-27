#include "MainComponent.h"

//==============================================================================
const juce::File MainComponent::settingsFile (juce::File::getSpecialLocation (juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile ("ntlabOscillatorDemoSettings.xml"));

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
    engineSelectionBox.setTextWhenNothingSelected ("Choose an Engine");
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
            if (deviceManager.selectEngine (selectedEngine))
            {
                std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (settingsFile));
                if (xml != nullptr)
                {
                    auto lastConfig = juce::ValueTree::fromXml (*xml);
                    auto settingConfig = deviceManager.getSelectedEngine()->setConfig (lastConfig);
                    if (settingConfig.wasOk())
                        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::InfoIcon, "Restored Engine settings", "Successfully restored engine settings from last session");
                    else
                        DBG (settingConfig.getErrorMessage());
                }
            }
        }
    };

    engineConfigButton.onClick = [this]()
    {
        if (deviceManager.getSelectedEngine())
        {
            engineConfigWindow = new EngineConfigWindow (deviceManager);
            engineConfigWindow->setBounds(20, 50, 600, 600);
            engineConfigWindow->setResizable (true, false);
            engineConfigWindow->setUsingNativeTitleBar (true);
            engineConfigWindow->setVisible (true);
        }
        else
            juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "No engine selected", "Select an Engine to configure");
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
                setUpEngine();
            else
                juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "No engine selected", "Select an Engine to start streaming");
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
    setupSliderRanges (initialCenterFreq);

    setSize (600, 400);
}

MainComponent::~MainComponent()
{
    deviceManager.stopStreaming();
    auto activeConfig = deviceManager.getSelectedEngine()->getActiveConfig();
    std::unique_ptr<juce::XmlElement> xml (activeConfig.createXml());
    if (xml != nullptr)
        xml->writeToFile (settingsFile, "");
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
    std::cout << "Starting to stream with " << numActiveChannelsIn << " input channels, " << numActiveChannelsOut << " output channels, block size " << maxNumSamplesPerBlock << " samples" << std::endl;

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
    auto* selectedEngine = deviceManager.getSelectedEngine();

    selectedEngine->enableRxTx (false, true);

    if (auto* hardwareEngine = dynamic_cast<ntlab::SDRIOHardwareEngine*> (selectedEngine))
    {
        hardwareEngine->addTuneChangeListener (&oscillator);

        bandwidth = hardwareEngine->getSampleRate();
        setupSliderRanges (hardwareEngine->getTxCenterFrequency (0));
    }

    deviceManager.setCallback (this);

    if (deviceManager.isReadyToStream())
    {
        setEngineState (true);
        deviceManager.startStreaming();
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::AlertIconType::WarningIcon, "Engine not ready to stream", "Streaming could not be started. Check the engine configuration");
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

void MainComponent::setupSliderRanges (double centerFreq)
{
    centerFreqSlider.setRange (centerFreq - 0.03e9, centerFreq + 0.03e9, 1000);
    centerFreqSlider.setValue (centerFreq, juce::NotificationType::dontSendNotification);
    oscillatorFreqSlider.setRange (centerFreq, centerFreq + bandwidth, 10);
    oscillatorFreqSlider.setValue (centerFreq, juce::NotificationType::dontSendNotification);
}