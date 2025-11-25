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
    setSize(330, 550);
}

SubSaverAudioProcessorEditor::~SubSaverAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    //logoImage = juce::ImageCache::getFromMemory(BinaryData::logo_png, BinaryData::logo_pngSize);
}

void SubSaverAudioProcessorEditor::paint(juce::Graphics& g)
{
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED/BURGUNDY) - 310px
    // ═══════════════════════════════════════════════════════════
    auto upperBounds = getLocalBounds().removeFromTop(310);
    g.setColour(juce::Colour(0xff6b2c2c));
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND (GREY) - 50px
    // ═══════════════════════════════════════════════════════════
    auto logoBand = getLocalBounds().withY(310).withHeight(50);

    // Gradiente grigio elegante
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2a2a2a), 0, 310,
        juce::Colour(0xff1a1a1a), 0, 360,
        false));
    g.fillRect(logoBand);

    // Bordi superiore e inferiore
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 310, getWidth(), 310, 2.0f);  // Bordo superiore
    g.drawLine(0, 360, getWidth(), 360, 2.0f);  // Bordo inferiore

    // LOGO TEXT (placeholder - sostituisci con immagine se vuoi)
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("SubSaver", logoBand, juce::Justification::centred);

    // Invece del testo:
    /*if (logoImage.isValid())
    {
        g.drawImage(logoImage, logoBand.reduced(10).toFloat(),
            juce::RectanglePlacement::centred);
    }*/


    // Sottotitolo opzionale
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f, juce::Font::italic));
    g.drawText("Foldback Distortion", logoBand.withTrimmedTop(28), juce::Justification::centred);

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE/SLATE) - remaining height
    // ═══════════════════════════════════════════════════════════
    auto lowerBounds = getLocalBounds().withY(360).withHeight(getHeight() - 360);
    g.setColour(juce::Colour(0xff3d4a5c));
    g.fillRect(lowerBounds);

    // ═══════════════════════════════════════════════════════════
    // PLACEHOLDER TEXT for Disperser section 
    // ═══════════════════════════════════════════════════════════
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(14.0f, juce::Font::italic));
    g.drawText("Disperser section", lowerBounds.reduced(20),
        juce::Justification::centred, true);
}


void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - 310px
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(310);

    // Dry slider (left) - INGRANDITO
    auto dryArea = upperSection.removeFromLeft(60).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right) - INGRANDITO
    auto wetArea = upperSection.removeFromRight(60).reduced(8, 15);
    wetLabel.setBounds(wetArea.removeFromBottom(20));
    wetSlider.setBounds(wetArea);

    // Center area for knobs
    auto centerArea = upperSection.reduced(10, 15);

    // Top row knobs - INGRANDITI
    auto topRow = centerArea.removeFromTop(110);
    auto driveArea = topRow.removeFromLeft(topRow.getWidth() / 2);
    auto tiltArea = topRow;

    tiltLabel.setBounds(tiltArea.removeFromBottom(20));
    tiltSlider.setBounds(tiltArea.reduced(5));

    driveLabel.setBounds(driveArea.removeFromBottom(20));
    driveSlider.setBounds(driveArea.reduced(5));

    centerArea.removeFromTop(10);

    // Bottom row knobs - INGRANDITI
    auto bottomRow = centerArea.removeFromTop(110);
    auto stereoArea = bottomRow.removeFromLeft(bottomRow.getWidth() / 2);
    auto outgainArea = bottomRow;

    stereoWidthLabel.setBounds(stereoArea.removeFromBottom(20));
    stereoWidthSlider.setBounds(stereoArea.reduced(5));

    envAmountLabel.setBounds(outgainArea.removeFromBottom(20));
    envAmountSlider.setBounds(outgainArea.reduced(5));

    centerArea.removeFromTop(15);

    // Distortion Type slider - INGRANDITO
    auto sliderArea = centerArea.removeFromTop(70);
    shapeModeLabel.setBounds(sliderArea.removeFromTop(20));
    shapeModeSlider.setBounds(sliderArea.reduced(20, 5));

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON
    // ═══════════════════════════════════════════════════════════
    oversamplingToggle.setBounds(getWidth() - 50, getHeight() - 35, 40, 25);
}

