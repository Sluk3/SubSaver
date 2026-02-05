#pragma once

#include <JuceHeader.h>

namespace Parameters
{
    // Nomi parametri
    static const juce::String nameDryLevel = "dryLevel";
    static const juce::String nameWetLevel = "wetLevel";
    static const juce::String nameDrive = "drive";
    static const juce::String nameStereoWidth = "stereoWidth";
    static const juce::String nameEnvAmount = "envAmount";
    static const juce::String nameShapeMode = "shapeMode";
    static const juce::String nameTilt = "colour";
    static const juce::String nameOversampling = "oversampling";
    static const juce::String nameDisperserAmount = "disperserAmount";
    static const juce::String nameDisperserFreq = "disperserFreq";
    static const juce::String nameDisperserPinch = "disperserPinch";

    // Default Values & Range
    static const float defaultDryLevel = 1.0f;
    static const float defaultWetLevel = 0.5f;
    static const float defaultDrive = 5.0f;
    static const float defaultStereoWidth = 0.0f;
    static const float defaultEnvAmount = 1.0f;
    static const int defaultShapeMode = 0; // A
    static const float defaultTilt = 0.0f;
    static const bool defaultOversampling = true;
    static const float defaultDisperserAmount = 0.0f;
    static const float defaultDisperserFreq = 1000.0f;
    static const float defaultDisperserPinch = 1.0f;

    // Crea il layout parametri
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;

        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        params.push_back(std::make_unique<AudioParameterFloat>(nameDryLevel, "Dry Level", 0.0f, 1.0f, defaultDryLevel));
        params.push_back(std::make_unique<AudioParameterFloat>(nameWetLevel, "Wet Level", 0.0f, 0.70f, defaultWetLevel));
        params.push_back(std::make_unique<AudioParameterFloat>(nameDrive, "Drive", NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), defaultDrive));
        params.push_back(std::make_unique<AudioParameterFloat>(nameStereoWidth, "Stereo Width", 0.0f, 0.25f, defaultStereoWidth));
        params.push_back(std::make_unique<AudioParameterFloat>(nameEnvAmount, "Env Amount", 0.0f, 1.0f, defaultEnvAmount));
        params.push_back(std::make_unique<AudioParameterChoice>(nameShapeMode, "Waveshaping Mode", StringArray{ "A", "B", "C", "D" }, defaultShapeMode));
        params.push_back(std::make_unique<AudioParameterFloat>(nameTilt, "Colour", -12.0f, 12.0f, defaultTilt));
        params.push_back(std::make_unique<AudioParameterBool>(nameOversampling, "Oversampling", defaultOversampling));
        params.push_back(std::make_unique<AudioParameterFloat>(nameDisperserAmount, "Disperser Amount", 0.0f, 1.0f, defaultDisperserAmount));
        params.push_back(std::make_unique<AudioParameterFloat>(nameDisperserFreq, "Disperser Frequency",NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), defaultDisperserFreq));
        params.push_back(std::make_unique<AudioParameterFloat>(nameDisperserPinch, "Disperser Pinch", 0.5f, 10.0f, defaultDisperserPinch));

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
