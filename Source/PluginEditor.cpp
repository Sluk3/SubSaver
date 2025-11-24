#include "PluginProcessor.h"
#include "PluginEditor.h"

SubSaverAudioProcessorEditor::SubSaverAudioProcessorEditor(SubSaverAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);

    // ═══════════════════════════════════════════════════════════
    // HELPER per configurare knobs
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
        label.setFont(juce::Font(12.0f));
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
        };

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) CONTROLS
    // ═══════════════════════════════════════════════════════════

    // Vertical sliders (Dry/Wet)
    setupVerticalSlider(drySlider);
    dryAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDryLevel, drySlider);

    setupVerticalSlider(wetSlider);
    wetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameWetLevel, wetSlider);

    // Top row knobs
    setupKnob(driveSlider); // drive
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDrive, driveSlider);

    setupKnob(tiltSlider); // "tilt"
    tiltAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.parameters, Parameters::nameTilt, tiltSlider);

    // Bottom row knobs
    setupKnob(stereoWidthSlider); // "Stereo"
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameStereoWidth, stereoWidthSlider);

    setupKnob(envAmountSlider); // "Env Amount"
    envAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameEnvAmount, envAmountSlider);

    // Horizontal slider (Distortion Type)
    setupHorizontalSlider(shapeModeSlider);
    shapeModeSlider.setRange(0, 3, 1); // 4 types: 0=A, 1=B, 2=C, 3=D
    shapeModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameShapeMode, shapeModeSlider);

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON (small, bottom right)
    // ═══════════════════════════════════════════════════════════
    oversamplingToggle.setButtonText("OS");
    oversamplingToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    oversamplingToggle.setTooltip("Oversampling On/Off");
    addAndMakeVisible(oversamplingToggle);
    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, Parameters::nameOversampling, oversamplingToggle);

    // ═══════════════════════════════════════════════════════════
    // LABELS
    // ═══════════════════════════════════════════════════════════
    setupLabel(dryLabel, "Dry Level");
    setupLabel(wetLabel, "Wet Level");
    setupLabel(tiltLabel, "Colour");
    setupLabel(driveLabel, "Drive");
    setupLabel(stereoWidthLabel, "Stereo");
    setupLabel(envAmountLabel, "Envelope Follower");
    setupLabel(shapeModeLabel, "Distortion Type");

    // ═══════════════════════════════════════════════════════════
    // SIZE (matching reference: ~330x460)
    // ═══════════════════════════════════════════════════════════
    setSize(330, 460);
}

SubSaverAudioProcessorEditor::~SubSaverAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void SubSaverAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED/BURGUNDY) - height ~270px
    // ═══════════════════════════════════════════════════════════
    auto upperBounds = getLocalBounds().removeFromTop(270);
    g.setColour(juce::Colour(0xff6b2c2c)); // Dark red/burgundy
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE/SLATE) - remaining height
    // ═══════════════════════════════════════════════════════════
    g.setColour(juce::Colour(0xff3d4a5c)); // Dark blue-grey
    g.fillRect(getLocalBounds().removeFromBottom(getHeight() - 270));

    // Border between sections
    g.setColour(juce::Colour(0xff1a1a1a));
    g.drawLine(0, 270, getWidth(), 270, 2.0f);

    // ═══════════════════════════════════════════════════════════
    // PLACEHOLDER TEXT for Disperser section (future implementation)
    // ═══════════════════════════════════════════════════════════
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(14.0f, juce::Font::italic));
    g.drawText("Disperser section",
        getLocalBounds().removeFromBottom(getHeight() - 270).reduced(20),
        juce::Justification::centred, true);
}

void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - 270px height
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(270);

    // Dry slider (left)
    auto dryArea = upperSection.removeFromLeft(45).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right)
    auto wetArea = upperSection.removeFromRight(45).reduced(8, 15);
    wetLabel.setBounds(wetArea.removeFromBottom(20));
    wetSlider.setBounds(wetArea);

    // Center area for knobs
    auto centerArea = upperSection.reduced(10, 15);

    // Top row knobs (Freq, Boost)
    auto topRow = centerArea.removeFromTop(90);
    auto freqArea = topRow.removeFromLeft(topRow.getWidth() / 2);
    auto boostArea = topRow;

    tiltLabel.setBounds(freqArea.removeFromBottom(20));
    tiltSlider.setBounds(freqArea.reduced(10));

    driveLabel.setBounds(boostArea.removeFromBottom(20));
    driveSlider.setBounds(boostArea.reduced(10));

    centerArea.removeFromTop(10); // spacing

    // Bottom row knobs (Stereo, Out gain)
    auto bottomRow = centerArea.removeFromTop(90);
    auto stereoArea = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    auto outgainArea = bottomRow;

    stereoWidthLabel.setBounds(stereoArea.removeFromBottom(20));
    stereoWidthSlider.setBounds(stereoArea.reduced(10));

    envAmountLabel.setBounds(outgainArea.removeFromBottom(20));
    envAmountSlider.setBounds(outgainArea.reduced(10));

    centerArea.removeFromTop(10); // spacing

    // Distortion Type slider (horizontal)
    auto sliderArea = centerArea.removeFromTop(50);
    shapeModeLabel.setBounds(sliderArea.removeFromTop(20));
    shapeModeSlider.setBounds(sliderArea.reduced(20, 5));

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON (small, bottom right corner)
    // ═══════════════════════════════════════════════════════════
    oversamplingToggle.setBounds(getWidth() - 50, getHeight() - 35, 40, 25);

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE) - Reserved for future Disperser
    // ═══════════════════════════════════════════════════════════
    // (vuoto per ora, pronto per implementazione futura)
}
