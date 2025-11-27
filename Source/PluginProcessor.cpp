#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginParameters.h"


//==============================================================================
SubSaverAudioProcessor::SubSaverAudioProcessor()
    : AbstractProcessor(), parameters(*this, nullptr, "SUBSAVER", Parameters::createParameterLayout()),
    dryWetter(Parameters::defaultDryLevel, Parameters::defaultWetLevel, 0),
    waveshaper(Parameters::defaultDrive,Parameters::defaultStereoWidth,Parameters::defaultOversampling),
    envelopeFollower(Parameters::defaultEnvAmount),
    tiltFilterPre(0.0f, 1000.0f),  
    tiltFilterPost(0.0f, 1000.0f),
    disperser(Parameters::defaultDisperserAmount, Parameters::defaultDisperserFreq, Parameters::defaultDisperserPinch)
{

    Parameters::addListenerToAllParameters(parameters, this);
}


SubSaverAudioProcessor::~SubSaverAudioProcessor() {}

//==============================================================================
void SubSaverAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	waveshaper.prepareToPlay(sampleRate,samplesPerBlock, getTotalNumOutputChannels());
    
    tiltFilterPre.prepareToPlay(sampleRate, samplesPerBlock);
    tiltFilterPost.prepareToPlay(sampleRate, samplesPerBlock);
    envelopeFollower.prepareToPlay(sampleRate);
    envelopeBuffer.setSize(1, samplesPerBlock);
    modulatedDriveBuffer.setSize(1, samplesPerBlock);
	disperser.prepareToPlay(sampleRate, samplesPerBlock);
    int totalLatency = calculateTotalLatency(sampleRate);
    // ════════════════════════════════════════════════
    // DEBUG: Stampa la latenza calcolata
    // ════════════════════════════════════════════════
    /*juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "SubSaver Debug",
        "Total Latency: " + juce::String(totalLatency) + " samples\n" +
        "Latency (ms): " + juce::String(totalLatency / sampleRate * 1000.0, 2) +
        ("Sample Rate: " + juce::String(sampleRate) + " Hz")+
        ("Block Size: " + juce::String(samplesPerBlock) + " samples")+
        ("Oversampling Latency: " + juce::String(foldback.getLatencySamples()) + " samples")+
        ("Tilt Pre Latency: " + juce::String(tiltFilterPre.getLatencySamples()) + " samples")+
        ("Tilt Post Latency: " + juce::String(tiltFilterPost.getLatencySamples()) + " samples")        
        ,
        "OK"
    );*/
    
    // Comunica la latenza all'host
    setLatencySamples(totalLatency);
    dryWetter.prepareToPlay(sampleRate, samplesPerBlock, totalLatency);

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
    waveshaper.processBlock(buffer,modulatedDriveBuffer);

    tiltFilterPost.processBlock(buffer,numSamples);
    // 5. Mixa dry/wet
    dryWetter.mergeDryAndWet(buffer);

	// 6. Disperser
	disperser.processBlock(buffer);
}

//==============================================================================
juce::AudioProcessorEditor* SubSaverAudioProcessor::createEditor()
{
    return new SubSaverAudioProcessorEditor(*this);
}

int SubSaverAudioProcessor::calculateTotalLatency(double sampleRate)
{
    int latency = 0;

    // Latenza oversampling (es. 4x con filtri FIR)
    
    latency += waveshaper.getLatencySamples();

    // Latenza filtri (dipende dall'ordine e tipo)
    latency += tiltFilterPre.getLatencySamples();
    latency += tiltFilterPost.getLatencySamples();

	//LATENZA DISPERSER
	//latency += disperser.getLatencySamples();
    return latency;
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
		waveshaper.setOversampling(static_cast<bool>(newValue));
		updateHostDisplay(juce::AudioProcessor::ChangeDetails().withLatencyChanged(true));
    }else if(parameterID == Parameters::nameShapeMode){
        waveshaper.setWaveshapeType(static_cast<WaveshapeType>(static_cast<int>(newValue)));
	}
    else if (parameterID == Parameters::nameDisperserAmount)
        disperser.setAmount(newValue);
    else if (parameterID == Parameters::nameDisperserFreq)
        disperser.setFrequency(newValue);
    else if (parameterID == Parameters::nameDisperserPinch)
        disperser.setPinch(newValue);

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