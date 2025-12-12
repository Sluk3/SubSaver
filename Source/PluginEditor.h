#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "BinaryData.h" 

// ════════════════════════════════════════════════════════════════════════════════
// SUBSAVER LITE EDITOR
// Simplified UI: Dry/Wet, Drive, Stereo Width, Oversampling only
// ════════════════════════════════════════════════════════════════════════════════

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
    
    // ═════════════════════════════════════════════════════════════════
    // SLIDERS (LITE VERSION)
    // ═════════════════════════════════════════════════════════════════
    juce::Slider drySlider;
    juce::Slider wetSlider;
    juce::Slider driveSlider;
    juce::Slider stereoWidthSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;

    // ═════════════════════════════════════════════════════════════════
    // LABELS
    // ═════════════════════════════════════════════════════════════════
    juce::Label dryLabel;
    juce::Label wetLabel;
    juce::Label driveLabel;
    juce::Label stereoWidthLabel;
    juce::Label titleLabel;

    // ═════════════════════════════════════════════════════════════════
    // LOGO & BUTTON
    // ═════════════════════════════════════════════════════════════════
    juce::Image logoImage;
    juce::ToggleButton oversamplingToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oversamplingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessorEditor)
};
