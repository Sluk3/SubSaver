#pragma once

#include <JuceHeader.h>

namespace Parameters
{
    // ═══════════════════════════════════════════════════════════
    // SUBSAVER LITE - SIMPLIFIED PARAMETERS
    // ═══════════════════════════════════════════════════════════
    
    // Nomi parametri
    static const juce::String nameDryLevel = "dryLevel";
    static const juce::String nameWetLevel = "wetLevel";
    static const juce::String nameDrive = "drive";
    static const juce::String nameStereoWidth = "stereoWidth";
    static const juce::String nameOversampling = "oversampling";

    // Default Values & Range
    static const float defaultDryLevel = 1.0f;
    static const float defaultWetLevel = 0.5f;
    static const float defaultDrive = 5.0f;
    static const float defaultStereoWidth = 0.0f;
    static const bool defaultOversampling = true;

    // Crea il layout parametri
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;

        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        params.push_back(std::make_unique<AudioParameterFloat>(nameDryLevel, "Dry Level", 0.0f, 1.0f, defaultDryLevel));
        params.push_back(std::make_unique<AudioParameterFloat>(nameWetLevel, "Wet Level", 0.0f, 0.70f, defaultWetLevel));
        params.push_back(std::make_unique<AudioParameterFloat>(nameDrive, "Drive", NormalisableRange<float>(0.0f, 10.0f, 0.01f, 0.3f), defaultDrive));
        params.push_back(std::make_unique<AudioParameterFloat>(nameStereoWidth, "Stereo Width", 0.0f, 0.25f, defaultStereoWidth));
        params.push_back(std::make_unique<AudioParameterBool>(nameOversampling, "Oversampling", defaultOversampling));

        return { params.begin(), params.end() };
    }
    
    static void addListenerToAllParameters(juce::AudioProcessorValueTreeState& valueTreeState, juce::AudioProcessorValueTreeState::Listener* listener)
    {
        std::unique_ptr<juce::XmlElement> xml(valueTreeState.state.createXml());

        for (auto* element : xml->getChildWithTagNameIterator("PARAM"))
        {
            const juce::String& id = element->getStringAttribute("id");
            valueTreeState.addParameterListener(id, listener);
        }
    }
}
