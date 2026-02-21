
/**
 * ═══════════════════════════════════════════════════════════════════════════
 * DISPERSER MODULE
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * Implementa una cascata di 16 filtri allpass per creare dispersione di fase
 * (group delay) senza alterare la risposta in ampiezza.
 *
 * ARCHITETTURA:
 * - 16 stadi di filtri allpass in serie per canale
 * - Distribuzione logaritmica delle frequenze lungo lo spettro
 * - Coefficient interpolation per eliminare zipper noise
 *
 * PARAMETRI:
 * - Amount [0-1]: Intensità dell'effetto (controlla il Q dei filtri)
 * - Frequency [20-20k Hz]: Frequenza centrale della dispersione
 * - Pinch [0.1-10]: Concentrazione dei filtri (alto=picco stretto, basso=wide)
 *
 * OTTIMIZZAZIONI:
 * - Bypass automatico quando amount < 0.005
 * - Calcolo coefficienti solo su cambiamenti significativi dei parametri
 */
#include "Filters.h"

class Disperser
{
public:
    static constexpr int MAX_STAGES = 16;

    Disperser(float defaultAmount = 0.0f, float defaultFrequency = 1000.0f, float defaultPinch = 1.0f)
        : currentAmount(defaultAmount)
        , currentFrequency(defaultFrequency)
        , currentPinch(defaultPinch)
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;

        // Prepara tutti i filtri
        for (int ch = 0; ch < 2; ++ch)
        {
            for (auto& filter : filters[ch])
            {
                filter.prepare(sampleRate);
            }
        }

        // Inizializza i coefficienti con i valori di default
        updateCoefficients(currentAmount, currentFrequency, currentPinch);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // Bypass ottimizzato se amount è quasi zero
        if (currentAmount < 0.005f)
        {
            return;
        }

        // Processing stereo
        for (int ch = 0; ch < numChannels && ch < 2; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);

            // Applica tutti i 16 stadi in cascata
            for (int stage = 0; stage < MAX_STAGES; ++stage)
            {
                filters[ch][stage].processBlock(channelData, numSamples);
            }
        }
    }

    void setAmount(float newAmount)
    {
        newAmount = juce::jlimit(0.0f, 1.0f, newAmount);
        if (std::abs(newAmount - currentAmount) > 0.001f)
        {
            currentAmount = newAmount;
            updateCoefficients(currentAmount, currentFrequency, currentPinch);
        }
    }

    void setFrequency(float newFrequency)
    {
        newFrequency = juce::jlimit(20.0f, 20000.0f, newFrequency);
        if (std::abs(newFrequency - currentFrequency) > 5.0f)
        {
            currentFrequency = newFrequency;
            updateCoefficients(currentAmount, currentFrequency, currentPinch);
        }
    }

    void setPinch(float newPinch)
    {
        newPinch = juce::jlimit(0.1f, 10.0f, newPinch);
        if (std::abs(newPinch - currentPinch) > 0.01f)
        {
            currentPinch = newPinch;
            updateCoefficients(currentAmount, currentFrequency, currentPinch);
        }
    }

    int getLatencySamples() const
    {
        return 0; // IIR filters have group delay but no fixed latency
    }

private:
    /**
     * Ricalcola e aggiorna i coefficienti di tutti i filtri della cascata
     */
    void updateCoefficients(float amount, float freq, float pinch)
    {
        float nyquist = static_cast<float>(sampleRate * 0.49);
        float safeFreq = juce::jlimit(20.0f, nyquist, freq);

        // Mapping amount con curva quadratica (risposta naturale)
        float amountCurved = amount * amount;

        // Calcolo Q con range dipendente dal pinch
        double minQ = 0.001;  // Soglia minima per stabilità
        double maxQ = 0.5 + (pinch * 0.5);
        double baseQ = minQ + amountCurved * (maxQ - minQ);

        // Distribuzione dei 16 filtri lungo lo spettro
        for (int i = 0; i < MAX_STAGES; ++i)
        {
            // Ratio normalizzato da 0.0 (primo filtro) a 1.0 (ultimo filtro)
            float ratio = (MAX_STAGES > 1) ? (float)i / (MAX_STAGES - 1) : 0.5f;

            // Spread logaritmico (in ottave)
            // Pinch alto = filtri concentrati, Pinch basso = filtri distribuiti
            float octaveSpread = 3.0f / pinch;
            float multiplier = std::pow(2.0f, (ratio - 0.5f) * octaveSpread);
            float stageFreq = juce::jlimit(20.0f, nyquist, safeFreq * multiplier);

            // Variazione del Q per stadio (evita risonanze troppo uniformi)
            double stageQ = baseQ * (0.8 + ratio * 0.4);

            // Aggiorna entrambi i canali con gli stessi valori (mono-compatible)
            filters[0][i].updateCoeffs(stageFreq, stageQ);
            filters[1][i].updateCoeffs(stageFreq, stageQ);
        }
    }

    double sampleRate = 44100.0;

    // Parametri correnti (no smoothing, gestito dall'interpolazione dei coefficienti)
    float currentAmount = 0.0f;
    float currentFrequency = 1000.0f;
    float currentPinch = 1.0f;

    // Array dei filtri: 2 canali (L/R) × 16 stadi in cascata
    std::array<std::array<BiquadAllpass, MAX_STAGES>, 2> filters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Disperser)
};
