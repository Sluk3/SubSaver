#include "PluginProcessor.h"
#include "PluginEditor.h"


SubSaverAudioProcessorEditor::SubSaverAudioProcessorEditor(SubSaverAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveformDisplay(p.parameters)
{
    setLookAndFeel(&customLookAndFeel);
    // ═══════════════════════════════════════════════════════════
    // CARICA FONT MONTSERRAT
    // ═══════════════════════════════════════════════════════════
    montserratFont = juce::Typeface::createSystemTypefaceFor(
        BinaryData::MontserratBold_ttf,
        BinaryData::MontserratBold_ttfSize
    );
    // ═══════════════════════════════════════════════════════════
    // HELPER FUNCTIONS
    // ═══════════════════════════════════════════════════════════
    auto setupKnob = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    auto setupVerticalSlider = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    auto setupHorizontalSlider = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);
        };

    auto setupLabel = [this](juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(montserratFont).withHeight(15.0f));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        };

    // ═══════════════════════════════════════════════════════════
    // LOGO IMAGE
    // ═══════════════════════════════════════════════════════════
    logoImage = juce::ImageFileFormat::loadFrom(
        BinaryData::SubSaverLogo_png,
        BinaryData::SubSaverLogo_pngSize
    );

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) CONTROLS
    // ═══════════════════════════════════════════════════════════

    // Vertical sliders
    setupVerticalSlider(drySlider);
    dryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDryLevel, drySlider);

    setupVerticalSlider(wetSlider);
    wetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameWetLevel, wetSlider);

    // Top row knobs
    setupKnob(driveSlider);
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDrive, driveSlider);

    setupKnob(tiltSlider);
    tiltAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameTilt, tiltSlider);

    // Bottom row knobs
    setupKnob(stereoWidthSlider);
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameStereoWidth, stereoWidthSlider);

    setupKnob(envAmountSlider);
    envAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameEnvAmount, envAmountSlider);

    // Horizontal slider
    setupHorizontalSlider(shapeModeSlider);
    shapeModeSlider.setRange(0, 3);
    shapeModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameMorph, shapeModeSlider);

    // Oversampling button

    oversamplingToggle.setButtonText("OS");
    oversamplingToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    oversamplingToggle.setTooltip("Oversampling On/Off");
    oversamplingToggle.setClickingTogglesState(true);  
    oversamplingToggle.setTriggeredOnMouseDown(false); 
    addAndMakeVisible(oversamplingToggle);
    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, Parameters::nameOversampling, oversamplingToggle);


    // Upper section labels
    setupLabel(dryLabel, "Dry Level");
    setupLabel(wetLabel, "Wet Level");
    setupLabel(tiltLabel, "Colour");
    setupLabel(driveLabel, "Drive");
    setupLabel(stereoWidthLabel, "Stereo");
    setupLabel(envAmountLabel, "Envelope Follower");
    setupLabel(shapeModeLabel, "Distortion Type");
    distortionTitleLabel.setText("DISTORTION", juce::dontSendNotification);
    distortionTitleLabel.setJustificationType(juce::Justification::centred);
    distortionTitleLabel.setFont(juce::Font(montserratFont).withHeight(19.0f));
    distortionTitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(distortionTitleLabel);

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE) - DISPERSER CONTROLS
    // ═══════════════════════════════════════════════════════════

    // Disperser knobs
    setupKnob(disperserAmountSlider);
    disperserAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDisperserAmount, disperserAmountSlider);

    setupHorizontalSlider(disperserFreqSlider);  
    disperserFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDisperserFreq, disperserFreqSlider);

    setupKnob(disperserPinchSlider);
    disperserPinchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDisperserPinch, disperserPinchSlider);

    // Disperser labels
    setupLabel(disperserAmountLabel, "Amount");
    setupLabel(disperserFreqLabel, "Frequency");
    setupLabel(disperserPinchLabel, "Pinch");

    // Label per mostrare il valore della frequenza
    disperserFreqValueLabel.setJustificationType(juce::Justification::centred);
    disperserFreqValueLabel.setFont(juce::Font(montserratFont).withHeight(14.0f));
    disperserFreqValueLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
    addAndMakeVisible(disperserFreqValueLabel);

    // Listener per aggiornare la label quando lo slider cambia
    disperserFreqSlider.onValueChange = [this]() {
        float freq = disperserFreqSlider.getValue();
        juce::String freqText;
        
        if (freq >= 1000.0f)
            freqText = juce::String(freq / 1000.0f, 2) + " kHz";
        else
            freqText = juce::String((int)freq) + " Hz";
        
        disperserFreqValueLabel.setText(freqText, juce::dontSendNotification);
    };

    // Inizializza il valore della label
    disperserFreqSlider.onValueChange();

    // Title label per la sezione disperser
    disperserTitleLabel.setText("DISPERSER", juce::dontSendNotification);
    disperserTitleLabel.setJustificationType(juce::Justification::centred);
    disperserTitleLabel.setFont(juce::Font(montserratFont).withHeight(19.0f));
    disperserTitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(disperserTitleLabel);

    addAndMakeVisible(waveformDisplay);

    // ═══════════════════════════════════════════════════════════
    // SIZE
    // ═══════════════════════════════════════════════════════════
    setSize(330, 725);
}


SubSaverAudioProcessorEditor::~SubSaverAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::SubSaverLogo_png, BinaryData::SubSaverLogo_pngSize);
}

void SubSaverAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED/BURGUNDY)
    // ═══════════════════════════════════════════════════════════
    auto upperBounds = getLocalBounds().removeFromTop(415); 

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff4a0f0f), 0, 0,
        juce::Colour(0xff1f1c1c), 0, 415,
        false));
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND (GREY)
    // ═══════════════════════════════════════════════════════════
    auto logoBand = getLocalBounds().withY(415).withHeight(50); 

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2a2a2a), 0, 415,
        juce::Colour(0xff1a1a1a), 0, 465,  
        false));
    g.fillRect(logoBand);

    // Logo image
    if (logoImage.isValid())
    {
        g.drawImage(logoImage,
            logoBand.reduced(10).toFloat(),
            juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::Font(montserratFont).withHeight(24.0f));
        g.drawText("SubSaver", logoBand, juce::Justification::centred);
    }

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE/SLATE) - Disperser
    // ═══════════════════════════════════════════════════════════
    auto lowerBounds = getLocalBounds().withY(465).withHeight(getHeight() - 465); 

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1c1e1f), 0, 465,  
        juce::Colour(0xff0f1f4a), 0, getHeight(),
        false));
    g.fillRect(lowerBounds);
}



void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED)
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(415);

    // TITLE "DISTORTION" in alto
    distortionTitleLabel.setBounds(upperSection.removeFromTop(25).reduced(10, 3));

    upperSection.removeFromTop(3);

    // Dry slider (left)
    auto dryArea = upperSection.removeFromLeft(70).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right) 
    auto wetArea = upperSection.removeFromRight(75).reduced(8, 15);
    wetLabel.setBounds(wetArea.removeFromBottom(20));
    wetSlider.setBounds(wetArea);

    // Center area for knobs
    auto centerArea = upperSection.reduced(10, 10);

    // Top row knobs
    auto topRow = centerArea.removeFromTop(110);
    auto driveArea = topRow.removeFromLeft(topRow.getWidth() / 2);
    auto tiltArea = topRow;

    tiltLabel.setBounds(tiltArea.removeFromBottom(20));
    tiltSlider.setBounds(tiltArea.reduced(5));

    driveLabel.setBounds(driveArea.removeFromBottom(20));
    driveSlider.setBounds(driveArea.reduced(5));

    centerArea.removeFromTop(10);

    // Bottom row knobs - SWAPPED POSITIONS
    auto bottomRow = centerArea.removeFromTop(110);
    auto envAmountArea = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    auto stereoArea = bottomRow;

    envAmountLabel.setBounds(envAmountArea.removeFromBottom(20));
    envAmountSlider.setBounds(envAmountArea.reduced(5));

    stereoWidthLabel.setBounds(stereoArea.removeFromBottom(20));
    stereoWidthSlider.setBounds(stereoArea.reduced(5));


    centerArea.removeFromTop(15);

    // Distortion Type slider - Margini ridotti per dare spazio al thumb (24px largo)
    auto sliderArea = centerArea.removeFromTop(145);
    shapeModeLabel.setBounds(sliderArea.removeFromTop(22));
    shapeModeSlider.setBounds(sliderArea.removeFromTop(38).reduced(5, 3));
    sliderArea.removeFromTop(5);
    waveformDisplay.setBounds(sliderArea.reduced(5, 0));

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND - skip (Y: 350-400)
    // ═══════════════════════════════════════════════════════════
    bounds.removeFromTop(50);

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE) - DISPERSER
    // ═══════════════════════════════════════════════════════════
    auto lowerSection = bounds;

    lowerSection.reduce(20, 15);

    disperserTitleLabel.setBounds(lowerSection.removeFromTop(25));

    lowerSection.removeFromTop(10);

    // Frequency slider - Aumentato spazio verticale per la label del valore
    auto freqSliderArea = lowerSection.removeFromTop(80);  // Aumentato da 60 a 80
    disperserFreqLabel.setBounds(freqSliderArea.removeFromTop(20));
    disperserFreqSlider.setBounds(freqSliderArea.removeFromTop(37).reduced(10, 3));
    disperserFreqValueLabel.setBounds(freqSliderArea.removeFromTop(18));  // Label del valore

    lowerSection.removeFromTop(15);

    // 2 Knobs
    auto knobsArea = lowerSection.removeFromTop(120);
    int knobWidth = knobsArea.getWidth() / 2;

    auto amountArea = knobsArea.removeFromLeft(knobWidth);
    disperserAmountLabel.setBounds(amountArea.removeFromBottom(20));
    disperserAmountSlider.setBounds(amountArea.reduced(10));

    auto pinchArea = knobsArea;
    disperserPinchLabel.setBounds(pinchArea.removeFromBottom(20));
    disperserPinchSlider.setBounds(pinchArea.reduced(10));

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON - COORDINATE ASSOLUTE
    // ═══════════════════════════════════════════════════════════
    // Posiziona il bottone in basso a destra con coordinate fisse
    int buttonWidth = 40;
    int buttonHeight = 25;
    int marginRight = 8;
    int marginBottom = 8;

    oversamplingToggle.setBounds(
        getWidth() - buttonWidth - marginRight,    // X: larghezza totale - larghezza bottone - margine
        getHeight() - buttonHeight - marginBottom, // Y: altezza totale - altezza bottone - margine
        buttonWidth,
        buttonHeight
    );
    
    // Forza il bottone in primo piano per renderlo sempre cliccabile
    oversamplingToggle.toFront(false);
}



