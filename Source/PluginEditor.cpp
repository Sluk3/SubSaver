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

    // --- SETUP STEREO SLIDER (AGGIUNTO) ---
    
    setupKnob(stereoWidthSlider);
    // NOTA: Controlla che "stereo" sia l'ID esatto usato in createParameterLayout() nel processore
    stereoWidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, Parameters::nameDrive, stereoWidthSlider);

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
  
    setupLabel(driveLabel, "Drive");
    setupLabel(stereoWidthLabel, "Stereo");
   
   
   

    

    // ═══════════════════════════════════════════════════════════
    // SIZE
    // ═══════════════════════════════════════════════════════════
    setSize(330, 400);
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
    auto upperBounds = getLocalBounds().removeFromTop(350);  

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff4a0f0f), 0, 0,
        juce::Colour(0xff1f1c1c), 0, 350, 
        false));
    g.fillRect(upperBounds);

    // ═══════════════════════════════════════════════════════════
    // LOGO BAND (GREY) 
    // ═══════════════════════════════════════════════════════════
    auto logoBand = getLocalBounds().withY(350).withHeight(50); 

    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff2a2a2a), 0, 350,  
        juce::Colour(0xff1a1a1a), 0, 400,  
        false));
    g.fillRect(logoBand);

    // Bordi
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 350, getWidth(), 350, 2.0f); 
    g.setColour(juce::Colour(0xff0a0a0a));
    g.drawLine(0, 400, getWidth(), 400, 2.0f);  

    // Logo image
    if (logoImage.isValid())
    {
        g.drawImage(logoImage,
            logoBand.reduced(10).toFloat(),
            juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }

}



void SubSaverAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - 350px
    // ═══════════════════════════════════════════════════════════
    auto upperSection = bounds.removeFromTop(350);

    upperSection.removeFromTop(3);

    // Dry slider (left)
    auto dryArea = upperSection.removeFromLeft(60).reduced(8, 15);
    dryLabel.setBounds(dryArea.removeFromBottom(20));
    drySlider.setBounds(dryArea);

    // Wet slider (right) 
    auto wetArea = upperSection.removeFromRight(65).reduced(8, 15);
    wetLabel.setBounds(wetArea.removeFromBottom(20));
    wetSlider.setBounds(wetArea);

    // Center area for knobs
    auto centerArea = upperSection.reduced(10, 10);

    // Top row knob
    auto topRow = centerArea.removeFromTop(110);
    auto driveArea = topRow;

    driveLabel.setBounds(driveArea.removeFromBottom(20));
    driveSlider.setBounds(driveArea.reduced(5));

    centerArea.removeFromTop(10);

    // Bottom row knob
    auto bottomRow = centerArea.removeFromTop(110);
    auto stereoArea = bottomRow;

    stereoWidthLabel.setBounds(stereoArea.removeFromBottom(10));
    stereoWidthSlider.setBounds(stereoArea.reduced(5));


    centerArea.removeFromTop(15);


    // ═══════════════════════════════════════════════════════════
    // LOGO BAND 
    // ═══════════════════════════════════════════════════════════
    bounds.removeFromTop(50);

    

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



