#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "BinaryData.h" 

class SubSaverAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    SubSaverAudioProcessorEditor(SubSaverAudioProcessor&);
    ~SubSaverAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SubSaverLookAndFeel customLookAndFeel;
    SubSaverAudioProcessor& audioProcessor;
    juce::Typeface::Ptr montserratFont;
    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - Main Distortion
    // ═══════════════════════════════════════════════════════════
    juce::Slider drySlider;
    juce::Slider wetSlider;
    juce::Slider tiltSlider;
    juce::Slider driveSlider;
    juce::Slider stereoWidthSlider;
    juce::Slider envAmountSlider;
    juce::Slider shapeModeSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tiltAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeModeAttachment;

    // ═══════════════════════════════════════════════════════════
    // LOWER SECTION (BLUE) - Disperser
    // ═══════════════════════════════════════════════════════════
    juce::Slider disperserAmountSlider;
    juce::Slider disperserFreqSlider;
    juce::Slider disperserPinchSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> disperserAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> disperserFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> disperserPinchAttachment;

    juce::Label disperserAmountLabel;
    juce::Label disperserFreqLabel;
    juce::Label disperserPinchLabel;
    juce::Label disperserTitleLabel;

    // ═══════════════════════════════════════════════════════════
    // LOGO IMAGE
    // ═══════════════════════════════════════════════════════════
    juce::Image logoImage;

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON
    // ═══════════════════════════════════════════════════════════
    juce::ToggleButton oversamplingToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oversamplingAttachment;

    // Labels upper section
    juce::Label dryLabel, wetLabel, tiltLabel, driveLabel,
        stereoWidthLabel, envAmountLabel, shapeModeLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessorEditor)
};
