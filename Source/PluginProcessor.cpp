#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"


//==============================================================================
SubSaverAudioProcessor::SubSaverAudioProcessor()
    : AbstractProcessor(), parameters(*this, nullptr, "SUBSAVER", Parameters::createParameterLayout()),
    dryWetter(Parameters::defaultDryLevel, Parameters::defaultWetLevel, 0),
    foldback(Parameters::defaultDrive)// <--- qui i valori di default
{
    Parameters::addListenerToAllParameters(parameters, this);
}


SubSaverAudioProcessor::~SubSaverAudioProcessor() {}

//==============================================================================
void SubSaverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	foldback.prepareToPlay(sampleRate);
    dryWetter.prepareToPlay(sampleRate, samplesPerBlock);
}

void SubSaverAudioProcessor::releaseResources()
{
    dryWetter.releaseResources();
}

void SubSaverAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals; // Non dimenticare!
    dryWetter.copyDrySignal(buffer);
	//distorsione e tutte le cose belle sul buffer
	foldback.processBlock(buffer);
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