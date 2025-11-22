#pragma once

#include <JuceHeader.h>
#include "AbstractProcessor.h"
#include "DryWet.h"
#include "Saturators.h"
#include "PluginParameters.h"
#include "EnvelopeFollower.h"
#include "Filters.h"
//==============================================================================


class SubSaverAudioProcessor : public AbstractProcessor
{
public:
    //==============================================================================
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
        return true; // (change this to false if you choose to not supply an editor)
    };

    //==============================================================================
    
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    DryWet dryWetter;
    WaveshaperCore waveshaper;
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    EnvelopeFollower envelopeFollower;
    juce::AudioProcessorValueTreeState parameters; 
    TiltFilter tiltFilterPre;  
    TiltFilter tiltFilterPost;  
    juce::AudioBuffer<double> envelopeBuffer;      // Envelope grezzo (0-1)
    juce::AudioBuffer<double> modulatedDriveBuffer; // Drive modulato
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessor)
};

