#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

// ════════════════════════════════════════════════════════════════════════════════
// SUBSAVER LITE - CONSTRUCTOR
// ════════════════════════════════════════════════════════════════════════════════
SubSaverAudioProcessor::SubSaverAudioProcessor()
    : AbstractProcessor(), 
      parameters(*this, nullptr, "SUBSAVER_LITE", Parameters::createParameterLayout()),
      dryWetter(Parameters::defaultDryLevel, Parameters::defaultWetLevel, 0),
      waveshaper(Parameters::defaultDrive, Parameters::defaultStereoWidth, Parameters::defaultOversampling)
{
    Parameters::addListenerToAllParameters(parameters, this);
}

SubSaverAudioProcessor::~SubSaverAudioProcessor() {}

// ════════════════════════════════════════════════════════════════════════════════
// PREPARE TO PLAY
// ════════════════════════════════════════════════════════════════════════════════
void SubSaverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepara il waveshaper (gestisce anche oversampling)
    waveshaper.prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    
    // Calcola la latenza totale e comunica all'host
    int totalLatency = calculateTotalLatency(sampleRate);
    setLatencySamples(totalLatency);
    
    // Prepara il dry/wet mixer con compensazione latenza
    dryWetter.prepareToPlay(sampleRate, samplesPerBlock, totalLatency);
}

void SubSaverAudioProcessor::releaseResources()
{
    dryWetter.releaseResources();
}

// ════════════════════════════════════════════════════════════════════════════════
// PROCESS BLOCK - SIMPLIFIED LITE VERSION
// ════════════════════════════════════════════════════════════════════════════════
// DSP CHAIN:
// Input → DRY/WET SPLIT → WAVEFOLDER (sine fold only) → MIX → Output
// ════════════════════════════════════════════════════════════════════════════════
void SubSaverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. Salva dry signal (con compensazione latenza)
    dryWetter.copyDrySignal(buffer);

    // 2. Applica wavefolder (Sine Fold) con stereo width
    //    Drive senza modulazione envelope
    waveshaper.processBlock(buffer);

    // 3. Mixa dry/wet
    dryWetter.mergeDryAndWet(buffer);
}

// ════════════════════════════════════════════════════════════════════════════════
// GUI EDITOR
// ════════════════════════════════════════════════════════════════════════════════
juce::AudioProcessorEditor* SubSaverAudioProcessor::createEditor()
{
    return new SubSaverAudioProcessorEditor(*this);
}

// ════════════════════════════════════════════════════════════════════════════════
// LATENCY CALCULATION
// ════════════════════════════════════════════════════════════════════════════════
int SubSaverAudioProcessor::calculateTotalLatency(double sampleRate)
{
    // Solo latenza dell'oversampling nel waveshaper
    return waveshaper.getLatencySamples();
}

// ════════════════════════════════════════════════════════════════════════════════
// PARAMETER CHANGED CALLBACK
// ════════════════════════════════════════════════════════════════════════════════
void SubSaverAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == Parameters::nameDryLevel)
        dryWetter.setDryLevel(newValue);
    else if (parameterID == Parameters::nameWetLevel)
        dryWetter.setWetLevel(newValue);
    else if (parameterID == Parameters::nameDrive) 
        waveshaper.setDrive(newValue);
    else if (parameterID == Parameters::nameStereoWidth) 
        waveshaper.setStereoWidth(newValue);  
    else if (parameterID == Parameters::nameOversampling) {
        waveshaper.setOversampling(static_cast<bool>(newValue));
        updateHostDisplay(juce::AudioProcessor::ChangeDetails().withLatencyChanged(true));
    }
}

// ════════════════════════════════════════════════════════════════════════════════
// PLUGIN INSTANTIATION
// ════════════════════════════════════════════════════════════════════════════════
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubSaverAudioProcessor();
}

// ════════════════════════════════════════════════════════════════════════════════
// STATE INFORMATION (PRESET SAVE/LOAD)
// ════════════════════════════════════════════════════════════════════════════════
void SubSaverAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SubSaverAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(ValueTree::fromXml(*xmlState));
}
