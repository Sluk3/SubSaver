#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"

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

    // ═══════════════════════════════════════════════════════════
    // UPPER SECTION (RED) - Main Distortion
    // ═══════════════════════════════════════════════════════════
    juce::Slider drySlider;           // Vertical left
    juce::Slider wetSlider;           // Vertical right
    juce::Slider tiltSlider;          // "Freq" knob (top left)
    juce::Slider driveSlider;         // "Boost" knob (top right)
    juce::Slider stereoWidthSlider;   // "Stereo" knob (bottom left)
    juce::Slider envAmountSlider;     // "Out gain" knob (bottom right)
    juce::Slider shapeModeSlider;     // Horizontal slider "Distortion Type"

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tiltAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> shapeModeAttachment;

    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLING BUTTON (bottom right corner)
    // ═══════════════════════════════════════════════════════════
    juce::ToggleButton oversamplingToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oversamplingAttachment;

    // Labels
    juce::Label dryLabel, wetLabel, tiltLabel, driveLabel,
        stereoWidthLabel, envAmountLabel, shapeModeLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessorEditor)
};
