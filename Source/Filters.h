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
 * DISPERSER MODULE
 *
 * Questo modulo implementa una "Allpass Cascade" (cascata di filtri allpass).
 * L'obiettivo è creare dispersione di fase (group delay) senza alterare la risposta in frequenza (magnitudine).
 *
 * PROBLEMA NOTO ("FRITTURA" / ZIPPER NOISE):
 * Quando il parametro 'Amount' viene mosso molto velocemente, si può udire un artefatto simile a una frittura/click.
 *
 * CAUSA TECNICA:
 * 1. Discontinuità di Stato: I filtri IIR (Infinite Impulse Response) mantengono una "memoria" (stati x1, x2, y1, y2).
 *    Quando cambiamo i coefficienti (b0, a1, etc.) al volo, l'energia immagazzinata nello stato precedente
 *    non corrisponde più matematicamente alla risposta del nuovo filtro. Questo crea un "salto" nel segnale in uscita.
 *
 * 2. Moltiplicazione dell'Errore: Abbiamo 16 filtri in serie. Un piccolo click generato dal cambio coefficienti
 *    nel filtro #1 viene amplificato, distorto e disperso dai successivi 15 filtri. L'errore si accumula esponenzialmente.
 *
 * 3. Cambio di Q drastico: L'amount controlla il fattore Q. Passare da Q=0.001 (filtro spento) a Q=0.5 (filtro attivo)
 *    sposta i poli del filtro molto velocemente. Questo rapido movimento dei poli è intrinsecamente rumoroso nei filtri digitali.
 *
 * SOLUZIONE ATTUALE (MITIGAZIONE):
 * - Ramp time molto lungo (500ms) per rallentare il movimento dei parametri.
 * - Curve di mapping (x^6) per rendere i cambi sui bassi valori quasi impercettibili.
 * - Control Rate a 64 samples per ridurre la frequenza degli aggiornamenti.
 */

class Disperser
{
public:
    // Numero fisso di filtri in serie. 16 stadi sono sufficienti per un effetto "laser/zappy" evidente.
    // Cambiare questo numero richiederebbe più CPU.
    static constexpr int MAX_STAGES = 16;

    // Per risparmiare CPU e ridurre gli artefatti, non ricalcoliamo i coefficienti trigonometrici (sin/cos)
    // per ogni singolo sample, ma ogni 64 sample (circa 1.4ms a 44.1kHz).
    static constexpr int CONTROL_RATE = 64;

    Disperser(float defaultAmount = 0.0f, float defaultFrequency = 1000.0f, float defaultPinch = 1.0f)
    {
        // Inizializza gli smoothers con i valori di default
        smoothedAmount.setCurrentAndTargetValue(defaultAmount);
        smoothedFrequency.setCurrentAndTargetValue(defaultFrequency);
        smoothedPinch.setCurrentAndTargetValue(defaultPinch);

        // Inizializza i tracker per il rilevamento dei cambiamenti
        lastAmount = defaultAmount;
        lastFrequency = defaultFrequency;
        lastPinch = defaultPinch;
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        this->sampleRate = sampleRate;

        // CONFIGURAZIONE SMOOTHING
		// Amount è il parametro più critico per gli artefatti
        smoothedAmount.reset(sampleRate, 0.05);

        // Freq e Pinch sono meno sensibili agli artefatti, 50ms è sufficiente per renderli fluidi.
        smoothedFrequency.reset(sampleRate, 0.05);
        smoothedPinch.reset(sampleRate, 0.05);

        // Reset completo della memoria dei filtri (stati x/y a zero) per evitare click all'avvio o cambio sample rate.
        for (int ch = 0; ch < 2; ++ch)
        {
            for (auto& filter : filters[ch])
            {
                filter.reset();
            }
        }

        // Allinea i valori di tracking
        lastAmount = smoothedAmount.getCurrentValue();
        lastFrequency = smoothedFrequency.getCurrentValue();
        lastPinch = smoothedPinch.getCurrentValue();
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        // OTTIMIZZAZIONE BYPASS:
        // Se l'amount è vicino a zero (sia il valore attuale che quello target), 
        // l'effetto è inudibile. Saltiamo tutto il calcolo DSP per risparmiare CPU.
        // Usiamo 0.005 come soglia di sicurezza.
        if (smoothedAmount.getCurrentValue() < 0.005f && smoothedAmount.getTargetValue() < 0.005f)
        {
            // Importante: anche se non processiamo, dobbiamo "avanzare" gli smoothers
            // per mantenerli sincronizzati col tempo reale.
            smoothedAmount.skip(numSamples);
            smoothedFrequency.skip(numSamples);
            smoothedPinch.skip(numSamples);
            return;
        }

        // PROCESSO A BLOCCHI (CONTROL RATE)
        int samplesProcessed = 0;

        while (samplesProcessed < numSamples)
        {
            // Determiniamo quanto grande è questo chunk (max 64 samples)
            int chunkSize = juce::jmin(CONTROL_RATE, numSamples - samplesProcessed);

            // 1. OTTENIAMO I VALORI DEI PARAMETRI
            // Usiamo getNextValue() per fare un passo dell'interpolazione lineare.
            float currentAmount = smoothedAmount.getNextValue();
            float currentFreq = smoothedFrequency.getNextValue();
            float currentPinch = smoothedPinch.getNextValue();

            // Saltiamo i restanti passi dello smoother per questo chunk, dato che usiamo
            // un valore costante per tutti i 64 sample del blocco.
            if (chunkSize > 1)
            {
                smoothedAmount.skip(chunkSize - 1);
                smoothedFrequency.skip(chunkSize - 1);
                smoothedPinch.skip(chunkSize - 1);
            }

            // 2. RILEVAMENTO CAMBIAMENTI (Optimization)
            // Ricalcolare 32 filtri (16*2 canali) richiede molta trigonometria (sin/cos/pow).
            // Lo facciamo solo se i parametri sono cambiati significativamente rispetto all'ultimo chunk.
            bool amountChanged = std::abs(currentAmount - lastAmount) > 0.002f; // Soglia alta per amount
            bool freqChanged = std::abs(currentFreq - lastFrequency) > 10.0f;   // Soglia Hz
            bool pinchChanged = std::abs(currentPinch - lastPinch) > 0.02f;     // Soglia pinch

            if (amountChanged || freqChanged || pinchChanged)
            {
                updateCoefficients(currentAmount, currentFreq, currentPinch);

                // Aggiorniamo i valori di riferimento
                lastAmount = currentAmount;
                lastFrequency = currentFreq;
                lastPinch = currentPinch;
            }

            // 3. DSP PROCESSING CORE
            for (int ch = 0; ch < numChannels && ch < 2; ++ch)
            {
                // Puntatore all'audio per questo specifico chunk
                float* channelData = buffer.getWritePointer(ch) + samplesProcessed;

                // Applichiamo TUTTI i 16 stadi in serie.
                // Non facciamo crossfade dry/wet o cambio numero stadi per evitare phasing.
                // L'intensità è gestita modulando il Q dei filtri dentro updateCoefficients.
                for (int stage = 0; stage < MAX_STAGES; ++stage)
                {
                    filters[ch][stage].processChunk(channelData, chunkSize);
                }
            }

            samplesProcessed += chunkSize;
        }
    }

    // Metodi per impostare i target dei parametri (chiamati dall'interfaccia/automazione)
    void setAmount(float newAmount)
    {
        smoothedAmount.setTargetValue(juce::jlimit(0.0f, 1.0f, newAmount));
    }

    void setFrequency(float newFrequency)
    {
        smoothedFrequency.setTargetValue(juce::jlimit(20.0f, 20000.0f, newFrequency));
    }

    void setPinch(float newPinch)
    {
        smoothedPinch.setTargetValue(juce::jlimit(0.1f, 10.0f, newPinch));
    }

    // La latenza di sistema è 0 perché usiamo filtri IIR (feedback).
    // C'è group delay (ritardo dipendente dalla frequenza), ma non latenza fissa.
    int getLatencySamples() const
    {
        return 0;
    }

private:
    // CLASSE INTERNA: BIQUAD ALLPASS
    // Implementa la formula standard Direct Form II o Transposed Direct Form II per un allpass del 2° ordine.
    class BiquadAllpass
    {
    public:
        void reset()
        {
            x1 = x2 = y1 = y2 = 0.0;
            // Default: passthrough perfetto (Unity Gain)
            b0 = 1.0; b1 = 0.0; b2 = 0.0;
            a1 = 0.0; a2 = 0.0;
        }

        void updateCoeffs(float freq, float Q, double sampleRate)
        {
            // SAFEGUARD: Se il Q è estremamente basso, il filtro è matematicamente instabile o inutile.
            // In questo caso lo forziamo a essere un cavo perfetto (Unity Gain).
            if (Q < 0.002)
            {
                b0 = 1.0; b1 = 0.0; b2 = 0.0; a1 = 0.0; a2 = 0.0;
                return;
            }

            // CALCOLO COEFFICIENTI ALLPASS 2° ORDINE
            // Formule standard RBJ Audio EQ Cookbook
            double omega = juce::MathConstants<double>::twoPi * freq / sampleRate;
            double sn = std::sin(omega);
            double cs = std::cos(omega);
            double alpha = sn / (2.0 * Q); // Alpha determina la larghezza di banda

            double a0 = 1.0 + alpha;
            double invA0 = 1.0 / a0; // Precalcolo l'inverso per usare moltiplicazioni (più veloci)

            // Per un Allpass:
            // Numeratore:   1 - alpha, -2cos, 1 + alpha
            // Denominatore: 1 + alpha, -2cos, 1 - alpha
            // Nota come i coefficienti sono simmetrici/scambiati rispetto ad altri filtri.
            b0 = (1.0 - alpha) * invA0;
            b1 = (-2.0 * cs) * invA0;
            b2 = (1.0 + alpha) * invA0;
            a1 = b1; // In un allpass, coefficiente a1 è uguale a b1
            a2 = b0; // In un allpass, coefficiente a2 è uguale a b0
        }

        // Processa un blocco di campioni (Loop DSP)
        void processChunk(float* data, int numSamples)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                double input = data[i];

                // Equazione alle differenze (Direct Form I o II normalizzata):
                // y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
                double output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;

                // ANTI-DENORMAL:
                // Quando i numeri diventano troppo piccoli (es. 1.0e-300), la CPU rallenta drasticamente.
                // Se il segnale è sotto questa soglia, lo forziamo a zero assoluto.
                if (std::abs(output) < 1.0e-20) output = 0.0;

                // Shift degli stati per il prossimo campione
                x2 = x1;
                x1 = input;
                y2 = y1;
                y1 = output;

                data[i] = static_cast<float>(output);
            }
        }

    private:
        double b0, b1, b2, a1, a2; // Coefficienti
        double x1, x2, y1, y2;     // Memoria (stato)
    };

    // LOGICA DI MAPPING DEI PARAMETRI
    void updateCoefficients(float amount, float freq, float pinch)
    {
        float nyquist = static_cast<float>(sampleRate * 0.49);
        float safeFreq = juce::jlimit(20.0f, nyquist, freq);

        // CURVA DI RISPOSTA "AMOUNT" (Cruciale per evitare frittura)
        // Dobbiamo mappare l'input lineare (0.0 - 1.0) su una curva che cresce MOLTO lentamente all'inizio.
        float amountCurved;

        // Zona critica (0% - 30%): Qui avviene la maggior parte dei problemi di "click".
        // Usiamo una curva polinomiale di 6° grado (x^6). 
        // Significa che se amount è 0.15 (metà zona), l'effetto è (0.5)^6 = 0.015 (1.5%).
        // Questo rende il cambiamento dei coefficienti minuscolo in questa zona sensibile.
        if (amount < 0.30f)
        {
            float t = amount / 0.30f;  // Normalizza 0-0.30 a 0.0-1.0
            amountCurved = t * t * t * t * t * t * 0.05f;  // Scala il risultato a max 0.05
        }
        else
        {
            // Sopra il 30%, usiamo una curva più standard (cubica) per una risposta musicale.
            float t = (amount - 0.30f) / 0.70f;
            amountCurved = 0.05f + (t * t * t) * 0.95f;
        }

        // MAPPARE AMOUNT A FATTRE Q
        // Minimo Q = 0.0005 (praticamente zero dispersione).
        // Massimo Q dipende dal "Pinch" (più pinch = picchi più risonanti = più dispersione).
        double minQ = 0.0005;
        double maxQ = 0.5 + (pinch * 0.5);

        // Interpoliamo il Q base per tutti i filtri
        double baseQ = minQ + amountCurved * (maxQ - minQ);

        // DISTRIBUZIONE DEI 16 FILTRI
        for (int i = 0; i < MAX_STAGES; ++i)
        {
            // Ratio va da 0.0 (primo filtro) a 1.0 (ultimo filtro)
            float ratio = (MAX_STAGES > 1) ? (float)i / (MAX_STAGES - 1) : 0.5f;

            // Spread Logaritmico delle frequenze
            // Pinch controlla quanto sono "sparpagliati" i filtri attorno alla frequenza centrale.
            // Pinch alto = filtri vicini (picco forte). Pinch basso = filtri larghi (dispersione wide).
            float octaveSpread = 3.0f / pinch;
            float multiplier = std::pow(2.0f, (ratio - 0.5f) * octaveSpread);

            float stageFreq = juce::jlimit(20.0f, nyquist, safeFreq * multiplier);

            // Variazione del Q per stadio
            // I filtri non hanno tutti lo stesso Q identico per evitare risonanze artificiali troppo metalliche.
            double stageQ = baseQ * (0.8 + ratio * 0.4);

            // Aggiorna L e R con gli stessi identici valori (effetto mono-compatibile)
            filters[0][i].updateCoeffs(stageFreq, stageQ, sampleRate);
            filters[1][i].updateCoeffs(stageFreq, stageQ, sampleRate);
        }
    }

    // Dati Membri
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFrequency;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedPinch;

    double sampleRate = 44100.0;

    // Valori per il change detection
    float lastAmount = 0.0f;
    float lastFrequency = 1000.0f;
    float lastPinch = 1.0f;

    // Matrice dei filtri: 2 canali x 16 stadi
    std::array<std::array<BiquadAllpass, MAX_STAGES>, 2> filters;
};
