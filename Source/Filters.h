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
        sampleRate(44100.0),
        Q(0.707f)
    {
        tiltAmount.setCurrentAndTargetValue(defaultTiltAmount);
    }

    void prepareToPlay(double sr, int maxBlockSize)
    {
        sampleRate = sr;
        tiltAmount.reset(sr, 0.005); // Smooth parameter changes
        tiltAmount.setCurrentAndTargetValue(Parameters::defaultTilt);
		lastTiltAmount = tiltAmount.getCurrentValue();
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
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
    {
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            // Ottieni il prossimo valore smoothato
            float currentTilt = tiltAmount.getNextValue();

            // Aggiorna i coefficienti solo se c'è un cambiamento significativo
            if (std::abs(currentTilt - lastTiltAmount) > 0.001f)
            {
                tiltAmount = currentTilt;
                updateCoefficients();
                lastTiltAmount = currentTilt;
            }

            // Processa ciascun canale per questo campione
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                float sample = channelData[i];

                // Applica low shelf e high shelf
                sample = lowShelf[ch].processSample(sample);
                sample = highShelf[ch].processSample(sample);
                sample *= (1 - std::abs(currentTilt) * 0.01f); // Compensa il guadagno totale
                channelData[i] = sample;
            }
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
	float lastTiltAmount = 0.0f;
    double sampleRate;
	float Q;
    // Filtri IIR per ogni canale (stereo)
    juce::dsp::IIR::Filter<float> lowShelf[2];
    juce::dsp::IIR::Filter<float> highShelf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TiltFilter)
};





/**
 * Disperser - Filtro di dispersione audio
 *
 * Un effetto di dispersione che utilizza una serie di filtri allpass
 * per creare un effetto di diffusione/spazializzazione del suono.
 *
 * Parametri:
 * - amount: 0.0 (nessuna dispersione) a 1.0 (massima dispersione)
 * - frequency: frequenza centrale della dispersione (20Hz - 20kHz)
 * - pinch: controlla la "concentrazione" della dispersione (0.1 - 10.0)
 *
 * Caratteristiche:
 * - Minimum phase (IIR)
 * - Latenza variabile in base al numero di filtri
 * - Stereo (2 canali indipendenti)
 */
#pragma once
#include <JuceHeader.h>
#include <vector>

class Disperser
{
public:
    Disperser(float defaultAmount = 0.0f, float defaultFrequency = 1000.0f, float defaultPinch = 1.0f)
        : amount(defaultAmount), frequency(defaultFrequency), pinch(defaultPinch)
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;

        // Crea processori per ogni canale
        for (int ch = 0; ch < 2; ++ch)
        {
            allpassChain[ch].clear();
        }

        updateFilters();
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (amount <= 0.0001f || allpassChain[0].empty())
            return;

        // Processa ogni canale
        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);

            // Passa il segnale attraverso ogni stadio allpass
            for (auto& allpass : allpassChain[ch])
            {
                allpass.process(channelData, numSamples);
            }
        }
    }

    void setAmount(float newAmount)
    {
        if (std::abs(amount - newAmount) > 0.001f)
        {
            amount = juce::jlimit(0.0f, 1.0f, newAmount);
            updateFilters();
        }
    }

    void setFrequency(float newFrequency)
    {
        if (std::abs(frequency - newFrequency) > 1.0f)
        {
            frequency = juce::jlimit(20.0f, 20000.0f, newFrequency);
            updateFilters();
        }
    }

    void setPinch(float newPinch)
    {
        if (std::abs(pinch - newPinch) > 0.01f)
        {
            pinch = juce::jlimit(0.1f, 10.0f, newPinch);
            updateFilters();
        }
    }

    int getLatencySamples() const
    {
        if (allpassChain[0].empty())
            return 0;

        // Ogni filtro aggiunge un po' di latenza
        int totalLatency = 0;
        for (const auto& ap : allpassChain[0])
        {
            totalLatency += ap.getLatency();
        }
        return totalLatency;
    }

private:
    // Semplice filtro allpass IIR del secondo ordine
    class BiquadAllpass
    {
    public:
        BiquadAllpass()
        {
            reset();
        }

        void setCoefficients(double freq, double Q, double sampleRate)
        {
            // Calcola i coefficienti per un allpass del secondo ordine
            double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
            double alpha = std::sin(omega) / (2.0 * Q);
            double cosOmega = std::cos(omega);

            // Normalizza
            double a0 = 1.0 + alpha;

            // Coefficienti allpass
            b0 = (1.0 - alpha) / a0;
            b1 = (-2.0 * cosOmega) / a0;
            b2 = (1.0 + alpha) / a0;
            a1 = (-2.0 * cosOmega) / a0;
            a2 = (1.0 - alpha) / a0;
        }

        void process(float* buffer, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                double input = buffer[i];

                // Direct Form II
                double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

                // Aggiorna stati
                x2 = x1;
                x1 = input;
                y2 = y1;
                y1 = output;

                buffer[i] = static_cast<float>(output);
            }
        }

        void reset()
        {
            x1 = x2 = y1 = y2 = 0.0;
            b0 = b1 = b2 = a1 = a2 = 0.0;
        }

        int getLatency() const
        {
            return 1; // Allpass biquad ha ~1 sample di group delay medio
        }

    private:
        double b0, b1, b2, a1, a2;
        double x1, x2, y1, y2;
    };

    void updateFilters()
    {
        if (sampleRate <= 0.0)
            return;

        // Mappa amount a numero di stadi (1-16)
        int numStages = juce::jmax(1, static_cast<int>(amount * 16.0f));

        if (amount <= 0.0001f)
        {
            for (int ch = 0; ch < 2; ++ch)
                allpassChain[ch].clear();
            return;
        }

        // Ricrea i filtri
        for (int ch = 0; ch < 2; ++ch)
        {
            allpassChain[ch].clear();
            allpassChain[ch].resize(numStages);
        }

        // Limita frequenza a range sicuro
        float nyquist = static_cast<float>(sampleRate) * 0.5f;
        float safeFreq = juce::jlimit(50.0f, nyquist * 0.9f, frequency);

        // Calcola Q base - valori più alti = picchi più stretti
        double baseQ = 0.5 + (pinch * 0.5); // Q tra 0.5 e 5.5

        // Crea la catena di filtri allpass
        for (int i = 0; i < numStages; ++i)
        {
            // Distribuzione logaritmica delle frequenze
            float ratio = numStages > 1 ? static_cast<float>(i) / (numStages - 1.0f) : 0.5f;

            // Spread delle frequenze controllato da pinch
            // Pinch alto = frequenze più concentrate, basso = più sparse
            float octaveSpread = 3.0f / pinch; // Spread in ottave
            float freqMultiplier = std::pow(2.0f, (ratio - 0.5f) * octaveSpread);

            float stageFreq = safeFreq * freqMultiplier;
            stageFreq = juce::jlimit(50.0f, nyquist * 0.9f, stageFreq);

            // Varia il Q leggermente per ogni stadio (più naturale)
            double stageQ = baseQ * (0.8 + ratio * 0.4);

            // Configura i filtri per entrambi i canali (stessi coefficienti)
            for (int ch = 0; ch < 2; ++ch)
            {
                allpassChain[ch][i].setCoefficients(stageFreq, stageQ, sampleRate);
            }
        }
    }

    float amount;
    float frequency;
    float pinch;
    double sampleRate = 0.0;

    // Catena di filtri allpass per ogni canale
    std::vector<BiquadAllpass> allpassChain[2];
};
