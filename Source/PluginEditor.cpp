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
    setSize(330, 620);
}


SubSaverAudioProcessorEditor::~SubSaverAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    logoImage = juce::ImageCache::getFromMemory(BinaryData::SubSaverLogo_png, BinaryData::SubSaverLogo_pngSize);
}

void SubSaverAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED/BURGUNDY) - 310px con GRADIENTE
    // ═══════════════════════════════════════════════════════════
    auto upperBounds = getLocalBounds().removeFromTop(320);

    // Gradiente rosso scuro esatto come richiesto
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff4a0f0f), 0, 0,
        juce::Colour(0xff1f1c1c), 0, 320,
        false));
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND (GREY) - 50px
    // ═══════════════════════════════════════════════════════════
    auto logoBand = getLocalBounds().withY(320).withHeight(50);

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2a2a2a), 0, 320,
        juce::Colour(0xff1a1a1a), 0, 370,
        false));
    g.fillRect(logoBand);

    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 320, getWidth(), 320, 2.0f);
	g.setColour(juce::Colour(0xff5a5a5a));
    g.drawLine(0, 370, getWidth(), 370, 2.0f);

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
    // LOWER SECTION (BLUE/SLATE) - Disperser con GRADIENTE BLU
    // ═══════════════════════════════════════════════════════════
    auto lowerBounds = getLocalBounds().withY(360).withHeight(getHeight() - 370);

    // Gradiente blu scuro (stessa logica del rosso)
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1c1e1f), 0, 370,            // Blu scuro in alto
        juce::Colour(0xff0f1f4a), 0, getHeight(),    // Quasi nero in basso
        false));
    g.fillRect(lowerBounds);
}



void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - AUMENTATA a 320px
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(320);  // Era 310, ora 320

    // Dry slider (left)
    auto dryArea = upperSection.removeFromLeft(60).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right)
    auto wetArea = upperSection.removeFromRight(60).reduced(8, 15);
    wetLabel.setBounds(wetArea.removeFromBottom(20));
    wetSlider.setBounds(wetArea);

    // Center area for knobs
    auto centerArea = upperSection.reduced(10, 15);

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

    // Distortion Type slider - ORA HA PIÙ SPAZIO
    auto sliderArea = centerArea.removeFromTop(75);  // Era 70, ora 75
    shapeModeLabel.setBounds(sliderArea.removeFromTop(20));
    shapeModeSlider.setBounds(sliderArea.reduced(20, 8));  // Margini verticali da 5 a 8

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND - skip (Y: 320-370)
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
    // OVERSAMPLING BUTTON
    // ═══════════════════════════════════════════════════════════
    oversamplingToggle.setBounds(getWidth() - 50, getHeight() - 35, 40, 25);
}




