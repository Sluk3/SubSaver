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

    void prepareToPlay(double sampleRate, int maxNumSamples, int numChannels, int maxDelay)
    {
        // VERIFICA parametri
        jassert(sampleRate > 0.0);
        jassert(maxNumSamples > 0);
        jassert(numChannels > 0);
        jassert(maxDelay >= 0);
        drySignal.setSize(numChannels, maxNumSamples);
        drySignal.clear();

        // Assicurati che il buffer sia abbastanza grande
        int safeDelaySize = juce::jmax(maxDelay, maxNumSamples * 2);

        delayBuffer.setSize(numChannels, safeDelaySize);
        delayBuffer.clear();
        delaySamples = maxDelay;
        
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

        // ═══════════════════════════════════════════════════════════════
        // DELAY COMPENSATION
        // ═══════════════════════════════════════════════════════════════
        if (delaySamples > 0)
        {
            const int delayBufferSize = delayBuffer.getNumSamples();

            // VERIFICA: controllo sicurezza
            if (delayBufferSize == 0 || delaySamples >= delayBufferSize)
            {
                jassertfalse;
                return;
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                // ═════════════════════════════════════════════════════════════
                // 1. SCRIVI nel circular buffer
                // ═════════════════════════════════════════════════════════════
                int samplesRemaining = numSamples;
                int sourceOffset = 0;
                int writePos = writePosition;

                while (samplesRemaining > 0)
                {
                    // Calcola quanti samples possiamo scrivere prima del wrap
                    int samplesUntilWrap = delayBufferSize - writePos;
                    int samplesToWrite = juce::jmin(samplesRemaining, samplesUntilWrap);

                    // VERIFICA: bounds check esplicito
                    jassert(writePos >= 0 && writePos < delayBufferSize);
                    jassert(samplesToWrite > 0 && samplesToWrite <= delayBufferSize);
                    jassert(sourceOffset >= 0 && sourceOffset + samplesToWrite <= numSamples);

                    // Scrivi il chunk
                    delayBuffer.copyFrom(ch, writePos, drySignal, ch, sourceOffset, samplesToWrite);

                    // Avanza i puntatori
                    samplesRemaining -= samplesToWrite;
                    sourceOffset += samplesToWrite;
                    writePos = (writePos + samplesToWrite) % delayBufferSize;
                }

                // ═════════════════════════════════════════════════════════════
                // 2. LEGGI dal circular buffer (con delay compensation)
                // ═════════════════════════════════════════════════════════════
                samplesRemaining = numSamples;
                int destOffset = 0;

                // Calcola read position con modulo sicuro
                int readPos = (writePosition - delaySamples + delayBufferSize) % delayBufferSize;

                // VERIFICA: read position valida
                jassert(readPos >= 0 && readPos < delayBufferSize);

                while (samplesRemaining > 0)
                {
                    // Calcola quanti samples possiamo leggere prima del wrap
                    int samplesUntilWrap = delayBufferSize - readPos;
                    int samplesToRead = juce::jmin(samplesRemaining, samplesUntilWrap);

                    // VERIFICA: bounds check esplicito
                    jassert(readPos >= 0 && readPos < delayBufferSize);
                    jassert(samplesToRead > 0 && samplesToRead <= delayBufferSize);
                    jassert(destOffset >= 0 && destOffset + samplesToRead <= numSamples);

                    // Leggi il chunk
                    drySignal.copyFrom(ch, destOffset, delayBuffer, ch, readPos, samplesToRead);

                    // Avanza i puntatori
                    samplesRemaining -= samplesToRead;
                    destOffset += samplesToRead;
                    readPos = (readPos + samplesToRead) % delayBufferSize;
                }
            }

            // 3. Aggiorna write position per il prossimo blocco
            writePosition = (writePosition + numSamples) % delayBufferSize;
        }

        // ═══════════════════════════════════════════════════════════════
        // DRY/WET MIXING
        // ═══════════════════════════════════════════════════════════════
        if (dryLevel.isSmoothing() || wetLevel.isSmoothing())
        {
            // FIX ZIPPER NOISE: Smoothing attivo: loop SAMPLE-first
            // Questo garantisce che getNextValue() sia chiamato una volta per sample,
            // non una volta per sample per channel (che causerebbe zipper noise)
            for (int i = 0; i < numSamples; ++i)
            {
                float dryGain = dryLevel.getNextValue();
                float wetGain = wetLevel.getNextValue();

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
            // Nessun smoothing: operazioni vettoriali
            float dryGain = dryLevel.getCurrentValue();
            float wetGain = wetLevel.getCurrentValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                wetBuffer.applyGain(ch, 0, numSamples, wetGain);
                wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples, dryGain);
            }
        }
    }


    void setDryLevel(float value) { dryLevel.setTargetValue(value); }
    void setWetLevel(float value) { wetLevel.setTargetValue(value); }
    void setDelaySamples(int samples)
    {
        // CONTROLLO SICUREZZA: non superare mai la dimensione del buffer
        int maxAllowedDelay = delayBuffer.getNumSamples() - 1;

        if (samples > maxAllowedDelay)
        {
            // Usa il massimo possibile senza crashare
            delaySamples = maxAllowedDelay;
        }
        else
        {
            delaySamples = juce::jlimit(0, maxAllowedDelay, samples);
        }
        delayBuffer.clear();
        writePosition = 0;
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
