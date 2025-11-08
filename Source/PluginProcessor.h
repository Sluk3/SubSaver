#pragma once

#include <JuceHeader.h>
#include "AbstractProcessor.h"
#include "DryWet.h"
#include "Saturators.h"
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

private:
    juce::AudioProcessorValueTreeState parameters; // FIX: usa la stessa inizializzazione del costruttore
    DryWet dryWetter;
    FoldbackSaturator foldback;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubSaverAudioProcessor)
};

