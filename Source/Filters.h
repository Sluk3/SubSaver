#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
#include <vector>
#include <array>

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
        sampleRate(44100.0),
        Q(0.707f)
    {
        tiltAmount.setCurrentAndTargetValue(defaultTiltAmount);
    }

    void prepareToPlay(double sr, int maxBlockSize)
    {
        sampleRate = sr;
        tiltAmount.reset(sr, 0.005);
        tiltAmount.setCurrentAndTargetValue(Parameters::defaultTilt);
        lastTiltAmount = tiltAmount.getCurrentValue();

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maximumBlockSize = maxBlockSize;
        spec.numChannels = 1;

        for (int ch = 0; ch < 2; ++ch)
        {
            lowShelf[ch].prepare(spec);
            highShelf[ch].prepare(spec);
            lowShelf[ch].reset();
            highShelf[ch].reset();
        }
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

    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float currentTilt = tiltAmount.getNextValue();

            if (std::abs(currentTilt - lastTiltAmount) > 0.001f)
            {
                tiltAmount = currentTilt;
                updateCoefficients();
                lastTiltAmount = currentTilt;
            }

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                float sample = channelData[i];

                sample = lowShelf[ch].processSample(sample);
                sample = highShelf[ch].processSample(sample);
                sample *= (1 - std::abs(currentTilt) * 0.01f);
                channelData[i] = sample;
            }
        }
    }

    int getLatencySamples() const
    {
        return 10;
    }

private:
    void updateCoefficients()
    {
        float currentTilt = tiltAmount.getNextValue();

        float lowGain = juce::Decibels::decibelsToGain(currentTilt);
        float highGain = juce::Decibels::decibelsToGain(-currentTilt);

        auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, pivotFrequency, Q, lowGain);

        auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, pivotFrequency, Q, highGain);

        for (int ch = 0; ch < 2; ++ch)
        {
            *lowShelf[ch].coefficients = *lowCoeffs;
            *highShelf[ch].coefficients = *highCoeffs;
        }
    }

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> tiltAmount;
    float pivotFrequency;
    float lastTiltAmount = 0.0f;
    double sampleRate;
    float Q;
    juce::dsp::IIR::Filter<float> lowShelf[2];
    juce::dsp::IIR::Filter<float> highShelf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TiltFilter)
};

/**
 * ═══════════════════════════════════════════════════════════════════════════
 * BIQUAD ALLPASS CON COEFFICIENT INTERPOLATION
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Filtro allpass del secondo ordine con interpolazione automatica dei coefficienti
 * per eliminare zipper noise e artefatti durante i cambi di parametri.
 *
 * CARATTERISTICHE:
 * - Risposta in ampiezza piatta (Unity Gain)
 * - Group delay dipendente dalla frequenza e Q
 * - Interpolazione lineare dei coefficienti su 64 samples (~1.5ms @ 44.1kHz)
 */
class BiquadAllpass
{
public:
    static constexpr int INTERP_SAMPLES = 64; // Durata dell'interpolazione

    BiquadAllpass()
    {
        reset();
    }

    /**
     * Prepara il filtro per l'audio processing
     * @param sr Sample rate in Hz
     */
    void prepare(double sr)
    {
        sampleRate = sr;
        reset();
    }

    /**
     * Resetta completamente il filtro (stati e coefficienti a unity gain)
     */
    void reset()
    {
        // Stati del filtro (memoria)
        x1 = x2 = y1 = y2 = 0.0;

        // Coefficienti correnti (unity gain passthrough)
        b0 = 1.0; b1 = 0.0; b2 = 0.0;
        a1 = 0.0; a2 = 0.0;

        // Coefficienti per interpolazione
        oldB0 = targetB0 = b0;
        oldB1 = targetB1 = b1;
        oldB2 = targetB2 = b2;
        oldA1 = targetA1 = a1;
        oldA2 = targetA2 = a2;

        interpolationCounter = INTERP_SAMPLES; // Non interpolare all'inizio
    }

    /**
     * Aggiorna i coefficienti del filtro (calcola i nuovi target e inizia l'interpolazione)
     * @param freq Frequenza centrale in Hz
     * @param Q Fattore di qualità (larghezza di banda)
     */
    void updateCoeffs(float freq, float Q)
    {
        // Safeguard per Q molto bassi (filtro quasi disabilitato)
        if (Q < 0.001)
        {
            // Imposta target per unity gain (filtro trasparente)
            targetB0 = 1.0; targetB1 = 0.0; targetB2 = 0.0;
            targetA1 = 0.0; targetA2 = 0.0;
        }
        else
        {
            // Calcola i nuovi coefficienti allpass usando RBJ Audio EQ Cookbook
            double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
            double sn = std::sin(omega);
            double cs = std::cos(omega);
            double alpha = sn / (2.0 * Q);

            double a0 = 1.0 + alpha;
            double invA0 = 1.0 / a0;

            // Coefficienti allpass 2° ordine
            // Nota: per un allpass, numeratore e denominatore sono speculari
            targetB0 = (1.0 - alpha) * invA0;
            targetB1 = (-2.0 * cs) * invA0;
            targetB2 = (1.0 + alpha) * invA0;
            targetA1 = targetB1;  // Simmetria allpass
            targetA2 = targetB0;  // Simmetria allpass
        }

        // Salva i coefficienti correnti come punto di partenza per l'interpolazione
        oldB0 = b0; oldB1 = b1; oldB2 = b2;
        oldA1 = a1; oldA2 = a2;

        // Inizia l'interpolazione da zero
        interpolationCounter = 0;
    }

    /**
     * Processa un blocco di campioni audio
     * @param data Puntatore ai sample da processare (in-place)
     * @param numSamples Numero di campioni da processare
     */
    void processBlock(float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            // ═══════════════════════════════════════════════════════════════
            // FASE 1: INTERPOLAZIONE DEI COEFFICIENTI
            // ═══════════════════════════════════════════════════════════════
            if (interpolationCounter < INTERP_SAMPLES)
            {
                // Calcola il fattore di interpolazione lineare [0.0 -> 1.0]
                float alpha = (float)(interpolationCounter + 1) / (float)INTERP_SAMPLES;

                // Interpola tutti i coefficienti simultaneamente
                // Questo mantiene la stabilità del filtro e previene artefatti
                b0 = oldB0 + alpha * (targetB0 - oldB0);
                b1 = oldB1 + alpha * (targetB1 - oldB1);
                b2 = oldB2 + alpha * (targetB2 - oldB2);
                a1 = oldA1 + alpha * (targetA1 - oldA1);
                a2 = oldA2 + alpha * (targetA2 - oldA2);

                interpolationCounter++;
            }

            // ═══════════════════════════════════════════════════════════════
            // FASE 2: DSP PROCESSING (BIQUAD DIFFERENCE EQUATION)
            // ═══════════════════════════════════════════════════════════════
            double input = data[i];

            // Equazione alle differenze del biquad (Direct Form I)
            // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
            double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

            // Anti-denormal: previene slowdown della CPU con numeri infinitesimi
            if (std::abs(output) < 1.0e-20)
                output = 0.0;

            // Aggiorna gli stati per il prossimo sample
            x2 = x1; x1 = input;
            y2 = y1; y1 = output;

            data[i] = static_cast<float>(output);
        }
    }

    /**
     * Processa un singolo sample (utile per real-time sample-by-sample processing)
     * @param sample Il sample da processare
     * @return Il sample processato
     */
    float processSample(float sample)
    {
        // Interpola se necessario
        if (interpolationCounter < INTERP_SAMPLES)
        {
            float alpha = (float)(interpolationCounter + 1) / (float)INTERP_SAMPLES;

            b0 = oldB0 + alpha * (targetB0 - oldB0);
            b1 = oldB1 + alpha * (targetB1 - oldB1);
            b2 = oldB2 + alpha * (targetB2 - oldB2);
            a1 = oldA1 + alpha * (targetA1 - oldA1);
            a2 = oldA2 + alpha * (targetA2 - oldA2);

            interpolationCounter++;
        }

        double input = sample;
        double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

        if (std::abs(output) < 1.0e-20)
            output = 0.0;

        x2 = x1; x1 = input;
        y2 = y1; y1 = output;

        return static_cast<float>(output);
    }

    /**
     * Verifica se il filtro sta ancora interpolando
     * @return true se l'interpolazione è in corso
     */
    bool isInterpolating() const
    {
        return interpolationCounter < INTERP_SAMPLES;
    }

private:
    double sampleRate = 44100.0;

    // Coefficienti correnti (usati nel processing)
    double b0, b1, b2, a1, a2;

    // Coefficienti per l'interpolazione
    double oldB0, oldB1, oldB2, oldA1, oldA2;           // Punto di partenza
    double targetB0, targetB1, targetB2, targetA1, targetA2;  // Destinazione

    int interpolationCounter = INTERP_SAMPLES;          // Contatore interpolazione

    // Stati del filtro (memoria)
    double x1, x2, y1, y2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BiquadAllpass)
};
