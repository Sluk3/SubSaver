#pragma once

#include <JuceHeader.h>
#include "AbstractProcessor.h"
#include "DryWet.h"
#include "Saturators.h"
#include "PluginParameters.h"
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

    bool hasEditor() const override {
        return false; // (change this to false if you choose to not supply an editor)
    };

    //==============================================================================
    
    void getStateInformation(MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    DryWet dryWetter;
    FoldbackSaturator foldback;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Aggiungi questo membro dati privato per risolvere l'errore E0292
    juce::AudioProcessorValueTreeState parameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessor)
};

