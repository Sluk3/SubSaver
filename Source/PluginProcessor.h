#pragma once

#include <JuceHeader.h>
#include "AbstractProcessor.h"
#include "DryWet.h"
#include "Saturators.h"
#include "PluginParameters.h"


class SubSaverAudioProcessor : public AbstractProcessor
{
public:
    SubSaverAudioProcessor();
    ~SubSaverAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;

    int calculateTotalLatency(double sampleRate);

    bool hasEditor() const override {
        return true;
    };

    //==============================================================================
    
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState parameters;


private:
    
    DryWet dryWetter;
    WaveshaperCore waveshaper;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessor)
};
