#pragma once

#include <JuceHeader.h>

class DryWet
{
public:
    DryWet(float defaultDry = 1.0f, float defaultWet = 1.0f, int defaultDelaySamples = 0)
        : delaySamples(defaultDelaySamples)
    {
        dryLevel.setCurrentAndTargetValue(defaultDry);
		wetLevel.setCurrentAndTargetValue(defaultWet);
    }

    ~DryWet() {}

    void prepareToPlay(double sampleRate, int maxNumSamples, int numChannels, int maxDelay = 4092)
    {
       // Alloca circular buffer
        const int minBufferSize = maxDelay + maxNumSamples;
        const int bufferSize = juce::nextPowerOfTwo(minBufferSize);
        drySignal.setSize(numChannels, maxNumSamples);
        drySignal.clear();
        // Alloca circular buffer per delay compensation
        // Deve contenere delaySamples + margine 
        delayBuffer.setSize(numChannels, maxDelay);
        delayBuffer.clear();
        writePosition = 0;
		dryLevel.reset(sampleRate, 0.01); 
		wetLevel.reset(sampleRate, 0.01);
    }

    void releaseResources()
    {
        drySignal.setSize(0, 0);
        delayBuffer.setSize(0, 0);
    }


    void copyDrySignal(const juce::AudioBuffer<float>& inputBuffer)
    {
        const int numChannels = inputBuffer.getNumChannels();
        const int numSamples = inputBuffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
            drySignal.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);
    }

    void mergeDryAndWet(juce::AudioBuffer<float>& wetBuffer)
    {
        const int numChannels = wetBuffer.getNumChannels();
        const int numSamples = wetBuffer.getNumSamples();

        // delay compensation sul dry (solo se delaySamples > 0)
        // NUOVO CODICE (operazioni vettoriali):
        if (delaySamples > 0)
        {
            const int delayBufferSize = delayBuffer.getNumSamples();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                // 1. SCRIVI con copyFrom (ottimizzato SIMD)
                const int space = delayBufferSize - writePosition;  //   CALCOLA SPAZIO DISPONIBILE

                if (numSamples <= space)
                {
                    //   SCRITTURA IN UN BLOCCO (veloce)
                    delayBuffer.copyFrom(ch, writePosition, drySignal, ch, 0, numSamples);
                }
                else
                {
                    //   SCRITTURA IN DUE BLOCCHI per wrap-around
                    delayBuffer.copyFrom(ch, writePosition, drySignal, ch, 0, space);
                    delayBuffer.copyFrom(ch, 0, drySignal, ch, space, numSamples - space);
                }

                // 2. LEGGI con copyFrom (ottimizzato SIMD)
                const int readStart = (writePosition - delaySamples + delayBufferSize) % delayBufferSize;

                if (readStart + numSamples <= delayBufferSize)
                {
                    //   LETTURA IN UN BLOCCO (veloce)
                    drySignal.copyFrom(ch, 0, delayBuffer, ch, readStart, numSamples);
                }
                else
                {
                    //   LETTURA IN DUE BLOCCHI per wrap-around
                    const int samplesBeforeWrap = delayBufferSize - readStart;
                    drySignal.copyFrom(ch, 0, delayBuffer, ch, readStart, samplesBeforeWrap);
                    drySignal.copyFrom(ch, samplesBeforeWrap, delayBuffer, ch, 0, numSamples - samplesBeforeWrap);
                }
            }

            writePosition = (writePosition + numSamples) % delayBufferSize;
        }
        
            //   CONTROLLA SE C'È SMOOTHING ATTIVO
          
            if (dryLevel.isSmoothing() || wetLevel.isSmoothing())
            {
                // Smoothing attivo - dobbiamo chiamare getNextValue() per ogni sample
                for (int i = 0; i < numSamples; ++i)
                {
                    // ← Chiamato UNA VOLTA per sample (non per canale!)
                    float dryGain = dryLevel.getNextValue();
                    float wetGain = wetLevel.getNextValue();

                    // Applica lo stesso gain a TUTTI i canali
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        float dry = drySignal.getSample(ch, i) * dryGain;
                        float wet = wetBuffer.getSample(ch, i) * wetGain;
                        wetBuffer.setSample(ch, i, dry + wet);
                    }
                }
            }
            else
            {
                // Valori stabili - usa operazioni vettoriali per efficienza
                float dryGain = dryLevel.getCurrentValue();
                float wetGain = wetLevel.getCurrentValue();

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    // Ottimizzazione: operazioni vettoriali
                    wetBuffer.applyGain(ch, 0, numSamples, wetGain);
                    wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples, dryGain);
                }
            }
        }
    

    void setDryLevel(float value) { dryLevel.setTargetValue(value); }
    void setWetLevel(float value) { wetLevel.setTargetValue(value); }
    void setDelaySamples(int samples)
    { 
        delaySamples = juce::jlimit(0, delayBuffer.getNumSamples() - 1, samples);
    }

private:
	SmoothedValue<float, ValueSmoothingTypes::Linear> dryLevel;
	SmoothedValue<float, ValueSmoothingTypes::Linear> wetLevel;
    int delaySamples;
	int writePosition = 0;
    juce::AudioBuffer<float> drySignal;
    juce::AudioBuffer<float> delayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DryWet);
};
