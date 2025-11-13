#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"


//==============================================================================
SubSaverAudioProcessor::SubSaverAudioProcessor()
    : AbstractProcessor(), parameters(*this, nullptr, "SUBSAVER", Parameters::createParameterLayout()),
    dryWetter(Parameters::defaultDryLevel, Parameters::defaultWetLevel, 0),
    foldback(Parameters::defaultDrive,Parameters::defaultStereoWidth,Parameters::defaultOversampling),
    envelopeFollower(Parameters::defaultEnvAmount),
    tiltFilterPre(0.0f, 1000.0f),  
    tiltFilterPost(0.0f, 1000.0f)
{

    Parameters::addListenerToAllParameters(parameters, this);
}


SubSaverAudioProcessor::~SubSaverAudioProcessor() {}

//==============================================================================
void SubSaverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	foldback.prepareToPlay(sampleRate,samplesPerBlock, getTotalNumOutputChannels());
    dryWetter.prepareToPlay(sampleRate, samplesPerBlock);
    tiltFilterPre.prepareToPlay(sampleRate, samplesPerBlock);
    tiltFilterPost.prepareToPlay(sampleRate, samplesPerBlock);
    envelopeFollower.prepareToPlay(sampleRate);
    envelopeBuffer.setSize(1, samplesPerBlock);
    modulatedDriveBuffer.setSize(1, samplesPerBlock);

}

void SubSaverAudioProcessor::releaseResources()
{
    dryWetter.releaseResources();
}

void SubSaverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; // Non dimenticare!

    const int numSamples = buffer.getNumSamples();



    // 1. Salva dry signal
    dryWetter.copyDrySignal(buffer);

    // 2. TILT FILTER PRE (modifica contenuto armonico prima della distorsione)
    tiltFilterPre.processBlock(buffer,numSamples);

    // 2. Genera envelope dal segnale (0-1)
    envelopeFollower.processBlock(buffer, envelopeBuffer);

    // 3. Copia envelope nel buffer di modulazione
    modulatedDriveBuffer.makeCopyOf(envelopeBuffer, true);

    // 4. Applica distorsione n drive modulato
    foldback.processBlock(buffer,modulatedDriveBuffer);

    tiltFilterPost.processBlock(buffer,numSamples);
    // 5. Mixa dry/wet
    dryWetter.mergeDryAndWet(buffer);
}

//==============================================================================
juce::AudioProcessorEditor* SubSaverAudioProcessor::createEditor()
{
    return nullptr; // Ritorna nullptr SE NON hai un editor (e hasEditor ritorna false)
}


void SubSaverAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == Parameters::nameDryLevel)
        dryWetter.setDryLevel(newValue);
    else if (parameterID == Parameters::nameWetLevel)
        dryWetter.setWetLevel(newValue);
  else if (parameterID == Parameters::nameDrive) 
        foldback.setDrive(newValue);
    else if (parameterID == Parameters::nameStereoWidth) 
        foldback.setStereoWidth(newValue);  
    else if (parameterID == Parameters::nameEnvAmount)
        envelopeFollower.setModAmount(newValue);
    else if (parameterID == Parameters::nameTilt)
    {
        // Tilt PRE: usa il valore diretto
        tiltFilterPre.setTiltAmount(newValue);

        // Tilt POST: inverte il valore (compensa)
        tiltFilterPost.setTiltAmount(-newValue);
    }
    else if (parameterID == Parameters::nameOversampling) {
		foldback.setOversampling(static_cast<bool>(newValue));
		updateHostDisplay(juce::AudioProcessor::ChangeDetails().withLatencyChanged(true));
    }
}


//==============================================================================
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