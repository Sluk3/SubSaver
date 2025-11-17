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

    void prepareToPlay(double sampleRate, int maxNumSamples, int maxDelay = 8192)
    {
        drySignal.setSize(2, maxNumSamples);
        drySignal.clear();
        // Alloca circular buffer per delay compensation
        // Deve contenere delaySamples + margine per il processing block
        delayBuffer.setSize(2, maxDelay);
        delayBuffer.clear();
        writePosition = 0;
		dryLevel.reset(sampleRate, 0.01); // Smoothing veloce
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
        if (delaySamples > 0)
        {
            const int delayBufferSize = delayBuffer.getNumSamples();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                // 1. SCRIVI i nuovi sample dry nel circular buffer
                for (int i = 0; i < numSamples; ++i)
                {
                    int writeIdx = (writePosition + i) % delayBufferSize;
                    delayBuffer.setSample(ch, writeIdx, drySignal.getSample(ch, i));
                }

				// 2. LEGGI i sample ritardati dal circular buffer + applica dry/wet levels
                for (int i = 0; i < numSamples; ++i)
                {
                    // Calcola la posizione di lettura (writePosition - delaySamples)
                    int readIdx = (writePosition + i - delaySamples + delayBufferSize) % delayBufferSize;
                    drySignal.setSample(ch, i, delayBuffer.getSample(ch, readIdx));

                    // Applica i livelli dry/wet smoothati
                    drySignal.applyGain(dryLevel.getNextValue());
                    wetBuffer.applyGain(wetLevel.getNextValue());
                }
            }

            // 3. AGGIORNA la write position per il prossimo blocco
            writePosition = (writePosition + numSamples) % delayBufferSize;
        }

        for (int ch = 0; ch < numChannels; ++ch)
            wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples);
    }

    void setDryLevel(float value) { dryLevel.setTargetValue(value); }
    void setWetLevel(float value) { wetLevel.setTargetValue(value); }
    void setDelaySamples(int samples)
    {
        delaySamples = samples;
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
