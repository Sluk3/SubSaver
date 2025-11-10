#pragma once

#include <JuceHeader.h>

//==============================================================================

class AbstractProcessor : public virtual juce::AudioProcessor,
    public virtual juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    AbstractProcessor() = default;
    //    : parameters(*this, nullptr) // Initialize `parameters` with required arguments
    //{
    //}
    virtual ~AbstractProcessor() override = default;

    //==============================================================================


    //===============METODI DA IMPLEMENTARE IN PLUGINPROCESSOR============

    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override = 0;
    virtual void releaseResources() override = 0;

    virtual void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override = 0;


    //==============================================================================
    virtual juce::AudioProcessorEditor* createEditor() override = 0;

    bool hasEditor() const override {
        return true; // (change this to false if you choose to not supply an editor)
    };

    //==============================================================================
    const juce::String getName() const override
    {
        return JucePlugin_Name;
    }

    bool acceptsMidi() const override
    {
#if JucePlugin_WantsMidiInput
        return true;
#else
        return false;
#endif
    }

    bool producesMidi() const override
    {
#if JucePlugin_ProducesMidiOutput
        return true;
#else
        return false;
#endif
    }

    bool isMidiEffect() const override
    {
#if JucePlugin_IsMidiEffect
        return true;
#else
        return false;
#endif
    }

    double getTailLengthSeconds() const override

    {
        return 0.0;
    }

    int getNumPrograms()
    {
        return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
        // so this should be at least 1, even if you're not really implementing programs.
    }

    int getCurrentProgram()
    {
        return 0;
    }

    void setCurrentProgram(int index)
    {
    }

    const juce::String getProgramName(int index)
    {
        return {};
    }

    void changeProgramName(int index, const juce::String& newName)
    {
    }

    virtual void getStateInformation(juce::MemoryBlock& destData) override = 0;

    virtual void setStateInformation(const void* data, int sizeInBytes) override = 0;



private:

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AbstractProcessor)


};
/*
template processor.h

#pragma once

#include <JuceHeader.h>
#include "AbstractProcessor.h"
//==============================================================================


class DelayAudioProcessor : public AbstractProcessor
{
public:
    //==============================================================================
    DelayAudioProcessor();
    ~DelayAudioProcessor() override;

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
    AudioProcessorValueTreeState parameters;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayAudioProcessor)
};
*/
///==============================================================================
/*
template processor.cpp

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"


//==============================================================================
DelayAudioProcessor::DelayAudioProcessor()
    : AbstractProcessor(), parameters(*this, nullptr, "DLY", Parameters::createParameterLayout())
{
    Parameters::addListenerToAllParameters(parameters, this);
}

DelayAudioProcessor::~DelayAudioProcessor(){}

//==============================================================================
void DelayAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{

}

void DelayAudioProcessor::releaseResources()
{

}

void DelayAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; // Non dimenticare!

}

//==============================================================================
juce::AudioProcessorEditor* DelayAudioProcessor::createEditor()
{
    return nullptr; // Ritorna nullptr SE NON hai un editor (e hasEditor ritorna false)
}


void DelayAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == Parameters::x)
    {

    }
    else if (parameterID == Parameters::y)
    {

    }
}


//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DelayAudioProcessor();
}
*/