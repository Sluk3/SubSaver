#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
/**
 * TiltFilter - Generic Tilt EQ Filter
 *
 * Un filtro tilt EQ che aumenta/diminuisce progressivamente
 * i bassi e gli alti attorno a una frequenza pivot.
 *
 * Implementato come combinazione di Low Shelf + High Shelf IIR.
 *
 * Parametri:
 * - tiltAmount: -12dB a +12dB (negativo = più bassi, positivo = più alti)
 * - pivotFreq: frequenza centrale dove il gain è 0dB (default 1kHz)
 *
 * Caratteristiche:
 * - Minimum phase (IIR)
 * - Latenza minima (~10-20 samples)
 * - Stereo (2 canali indipendenti)
 */
class TiltFilter
{
public:
    TiltFilter(float defaultTiltAmount = Parameters::defaultTilt, float defaultPivotFreq = 500.0f)
        : tiltAmount(defaultTiltAmount),
        pivotFrequency(defaultPivotFreq),
        sampleRate(44100.0)
    {
        tiltAmount.setCurrentAndTargetValue(defaultTiltAmount);
    }

    void prepareToPlay(double sr, int maxBlockSize)
    {
        sampleRate = sr;
        tiltAmount.reset(sr, 0.05); // Smooth parameter changes

        // Inizializza i filtri per stereo (2 canali)
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = maxBlockSize;
        spec.numChannels = 1; // Mono per ogni filtro

        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].prepare(spec);
            highShelf[ch].prepare(spec);
            lowShelf[ch].reset();
            highShelf[ch].reset();
        }

        updateCoefficients();
    }

    void setTiltAmount(float tiltDB)
    {
        tiltAmount.setTargetValue(juce::jlimit(-12.0f, 12.0f, tiltDB));
    }

    void setPivotFrequency(float freqHz)
    {
        pivotFrequency = juce::jlimit(100.0f, 10000.0f, freqHz);
        updateCoefficients();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].reset();
            highShelf[ch].reset();
        }
    }

    /**
     * Process a stereo audio buffer
     * Buffer deve avere 2 canali (L/R)
     */
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // Aggiorna coefficienti se il parametro è cambiato
        if (tiltAmount.isSmoothing())
        {
            updateCoefficients();
        }

        // Processa ogni canale indipendentemente
        for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);

            // Applica Low Shelf
            juce::dsp::AudioBlock<float> block(&channelData, 1, numSamples);
            juce::dsp::ProcessContextReplacing<float> contextLow(block);
            lowShelf[ch].process(contextLow);

            // Applica High Shelf
            juce::dsp::ProcessContextReplacing<float> contextHigh(block);
            highShelf[ch].process(contextHigh);
        }
    }

    /**
     * Ritorna la latenza stimata in samples
     * Per filtri IIR minimum-phase è molto bassa
     */
    int getLatencySamples() const
    {
        // Stima conservativa per IIR biquad (low + high shelf)
        return 10; // ~10 samples di group delay
    }

private:
    void updateCoefficients()
    {
        float currentTilt = tiltAmount.getNextValue();

        // Q factor per transizione smooth
        const float Q = 0.707f; // Butterworth response

        // Converti tilt amount in gain lineare
        float lowGain = juce::Decibels::decibelsToGain(currentTilt);
        float highGain = juce::Decibels::decibelsToGain(-currentTilt);

        // Crea coefficienti per entrambi i canali
        auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, pivotFrequency, Q, lowGain);

        auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, pivotFrequency, Q, highGain);

        // Applica a tutti i canali
        for (int ch = 0; ch < 2; ++ch)
        {
            *lowShelf[ch].coefficients = *lowCoeffs;
            *highShelf[ch].coefficients = *highCoeffs;
        }
    }

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> tiltAmount;
    float pivotFrequency;
    double sampleRate;

    // Filtri IIR per ogni canale (stereo)
    juce::dsp::IIR::Filter<float> lowShelf[2];
    juce::dsp::IIR::Filter<float> highShelf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TiltFilter)
};
