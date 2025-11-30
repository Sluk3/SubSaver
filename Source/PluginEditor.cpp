#include "PluginProcessor.h"
#include "PluginEditor.h"


SubSaverAudioProcessorEditor::SubSaverAudioProcessorEditor(SubSaverAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
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
    shapeModeSlider.setRange(0, 3, 1);
    shapeModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameShapeMode, shapeModeSlider);

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

    // Title label per la sezione disperser
    disperserTitleLabel.setText("DISPERSER", juce::dontSendNotification);
    disperserTitleLabel.setJustificationType(juce::Justification::centred);
    disperserTitleLabel.setFont(juce::Font(montserratFont).withHeight(19.0f));
    disperserTitleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(disperserTitleLabel);

    // ═══════════════════════════════════════════════════════════
    // SIZE
    // ═══════════════════════════════════════════════════════════
    setSize(330, 660);
}


SubSaverAudioProcessorEditor::~SubSaverAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::SubSaverLogo_png, BinaryData::SubSaverLogo_pngSize);
}

void SubSaverAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED/BURGUNDY) - AUMENTATA a 350px
    // ═══════════════════════════════════════════════════════════
    auto upperBounds = getLocalBounds().removeFromTop(350);  // Era 320, ora 350

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff4a0f0f), 0, 0,
        juce::Colour(0xff1f1c1c), 0, 350,  // Era 320, ora 350
        false));
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND (GREY) - 50px SPOSTATA PIÙ IN BASSO
    // ═══════════════════════════════════════════════════════════
    auto logoBand = getLocalBounds().withY(350).withHeight(50);  // Era Y:320, ora Y:350

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2a2a2a), 0, 350,  // Era 320, ora 350
        juce::Colour(0xff1a1a1a), 0, 400,  // Era 370, ora 400
        false));
    g.fillRect(logoBand);

    // Bordi
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 350, getWidth(), 350, 2.0f);  // Era 320, ora 350
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 400, getWidth(), 400, 2.0f);  // Era 370, ora 400

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
    auto lowerBounds = getLocalBounds().withY(400).withHeight(getHeight() - 400);  // Era 370, ora 400

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1c1e1f), 0, 400,  // Era 370, ora 400
        juce::Colour(0xff0f1f4a), 0, getHeight(),
        false));
    g.fillRect(lowerBounds);
}



void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - 350px
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(350);

    // TITLE "DISTORTION" in alto
    distortionTitleLabel.setBounds(upperSection.removeFromTop(25).reduced(10, 3));

    upperSection.removeFromTop(3);

    // Dry slider (left)
    auto dryArea = upperSection.removeFromLeft(60).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right) - AUMENTATA LARGHEZZA
    auto wetArea = upperSection.removeFromRight(65).reduced(8, 15);
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

    // Bottom row knobs
    auto bottomRow = centerArea.removeFromTop(110);
    auto stereoArea = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    auto outgainArea = bottomRow;

    stereoWidthLabel.setBounds(stereoArea.removeFromBottom(20));
    stereoWidthSlider.setBounds(stereoArea.reduced(5));

    envAmountLabel.setBounds(outgainArea.removeFromBottom(20));
    envAmountSlider.setBounds(outgainArea.reduced(5));

    centerArea.removeFromTop(15);

    // Distortion Type slider - MARGINI ORIZZONTALI RIDOTTI
    auto sliderArea = centerArea.removeFromTop(80);
    shapeModeLabel.setBounds(sliderArea.removeFromTop(22));
    shapeModeSlider.setBounds(sliderArea.reduced(10, 10));

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

    // Frequency slider
    auto freqSliderArea = lowerSection.removeFromTop(60);
    disperserFreqLabel.setBounds(freqSliderArea.removeFromTop(20));
    disperserFreqSlider.setBounds(freqSliderArea.reduced(15, 8));

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
}



