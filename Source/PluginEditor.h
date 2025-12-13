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
    
    juce::Slider drySlider;
    juce::Slider wetSlider;
    juce::Slider driveSlider;
    juce::Slider stereoWidthSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> wetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoWidthAttachment;

    juce::Label dryLabel;
    juce::Label wetLabel;
    juce::Label driveLabel;
    juce::Label stereoWidthLabel;
    juce::Label titleLabel;

    juce::Image logoImage;
    juce::ToggleButton oversamplingToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oversamplingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessorEditor)
};
