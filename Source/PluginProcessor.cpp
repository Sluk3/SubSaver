#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"

SubSaverAudioProcessor::SubSaverAudioProcessor()
    : AbstractProcessor(), 
      parameters(*this, nullptr, "SUBSAVER_LITE", Parameters::createParameterLayout()),
      dryWetter(Parameters::defaultDryLevel, Parameters::defaultWetLevel, 0),
      waveshaper(Parameters::defaultDrive, Parameters::defaultStereoWidth, Parameters::defaultOversampling)
{
    Parameters::addListenerToAllParameters(parameters, this);
}

SubSaverAudioProcessor::~SubSaverAudioProcessor() {}

void SubSaverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepara il waveshaper (gestisce anche oversampling)
    waveshaper.prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    
    int targetOversamplingFactor = static_cast<int>(TARGET_SAMPLING_RATE / sampleRate);
    targetOversamplingFactor = juce::jmin(16, juce::jmax(1, targetOversamplingFactor));

    // Latenza approssimativa: 64 samples per stage di oversampling
    int maxPossibleLatency = 64 * static_cast<int>(nextPowerOfTwo(targetOversamplingFactor));

    // Aggiungi margine di sicurezza (50% extra)
    int maxDelay = static_cast<int>(maxPossibleLatency * 1.5f);

    // Calcola la latenza attuale
    int totalLatency = waveshaper.getLatencySamples();
    setLatencySamples(totalLatency);

    // Prepara il dry/wet mixer con compensazione latenza
    dryWetter.prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels(), maxDelay);
}

void SubSaverAudioProcessor::releaseResources()
{
    dryWetter.releaseResources();
}

// DSP CHAIN:
// Input → DRY/WET SPLIT → WAVEFOLDER (sine fold) → MIX → Output
void SubSaverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. Salva dry signal (con compensazione latenza)
    dryWetter.copyDrySignal(buffer);

    // 2. Applica Sine Fold con stereo width
    waveshaper.processBlock(buffer);

    // 3. Mixa dry/wet
    dryWetter.mergeDryAndWet(buffer);
}

juce::AudioProcessorEditor* SubSaverAudioProcessor::createEditor()
{
    return new SubSaverAudioProcessorEditor(*this);
}

int SubSaverAudioProcessor::calculateTotalLatency(double sampleRate)
{
    // Solo latenza dell'oversampling nel waveshaper
    return waveshaper.getLatencySamples();
}

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
        // Ricalcola la latenza totale e aggiorna l'host
        int newLatency = calculateTotalLatency(getSampleRate());
        setLatencySamples(newLatency);

        // Aggiorna anche il dryWetter con la nuova latenza
        dryWetter.setDelaySamples(newLatency);
    }

}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SubSaverAudioProcessor();
}

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
