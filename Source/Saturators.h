#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"

#define TARGET_SAMPLING_RATE 192000.0

// ═══════════════════════════════════════════════════════════════
// ENUM per i tipi di distorsione (shape mode)
// ═══════════════════════════════════════════════════════════════
enum class WaveshapeType
{
    Chebyshev = 0,    // D: Chebyshev polynomial (armoniche dispari)
    SineFold = 1,     // A: Sine wavefolder (smooth, musicalità)
    Triangle = 2,     // B: Triangle wavefolder (geometrico)
    Foldback = 3     // C: Foldback classico (hard clipping piegato)
};

// ═══════════════════════════════════════════════════════════════
// WAVESHAPER CORE - Classe unificata modulare
// ═══════════════════════════════════════════════════════════════
class WaveshaperCore
{
public:
    WaveshaperCore(double defaultDrive = Parameters::defaultDrive, double defaultStereoWidth = Parameters::defaultStereoWidth, bool defaultOversampling = Parameters::defaultOversampling, WaveshapeType defaultType = WaveshapeType::SineFold)
        : drive(defaultDrive),
        stereoWidth(defaultStereoWidth),
        oversampling(defaultOversampling),
        currentType(defaultType)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    // ═══════════════════════════════════════════════════════════
    // SETUP & CONFIGURATION
    // ═══════════════════════════════════════════════════════════
    void prepareToPlay(double sampleRate, int samplesPerBlock, int numCh)
    {
        drive.reset(sampleRate, 0.03);
        stereoWidth.reset(sampleRate, 0.03);

        // DC blocker (HPF 5-7.5Hz)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 7.5);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = coeffs;
            dcBlocker[ch].reset();
        }

        maxSamplesPerBlock = samplesPerBlock;
        originalSampleRate = sampleRate;
        
        // FIX: Inizializza ENTRAMBI gli oversampler
        initOversamplers(samplesPerBlock);
    }

    void setWaveshapeType(WaveshapeType type)
    {
        if (currentType != type)
        {
            currentType = type;
            // Eventuale reset di stato per alcuni algoritmi
        }
    }

    void setOversampling(bool shouldOversample)
    {
        oversampling = shouldOversample;
        // FIX: NESSUN RESET - solo switch tra istanze già pronte
    }

    void setDrive(double value) { drive.setTargetValue(value); }
    void setStereoWidth(float width) { stereoWidth.setTargetValue(width); }

    // FIX: Ritorna latenza dell'istanza attualmente attiva
    int getLatencySamples() const noexcept
    {
        if (oversampling && oversamplerHigh)  // SELEZIONA L'ISTANZA CORRETTA
            return static_cast<int>(oversamplerHigh->getLatencyInSamples());
        else if (oversamplerBypass)  // ISTANZA BYPASS
            return static_cast<int>(oversamplerBypass->getLatencyInSamples());
        return 0;
    }

    // ═══════════════════════════════════════════════════════════
    // PROCESS BLOCK
    // ═══════════════════════════════════════════════════════════
    void processBlock(juce::AudioBuffer<float>& buffer,
        const juce::AudioBuffer<double>& envelopeBuffer)
    {
        // FIX: NESSUN RESET RUNTIME
        
        // FIX: SELEZIONE ISTANZA CORRETTA
        auto* activeOversampler = oversampling ? oversamplerHigh.get() : oversamplerBypass.get();
        const int activeFactor = oversampling ? oversamplingFactorHigh : 1;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        auto envData = envelopeBuffer.getReadPointer(0);

        // ═══════════════════════════════════════════════════════
        // OVERSAMPLING UP
        // ═══════════════════════════════════════════════════════
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        auto oversampledBlock = activeOversampler->processSamplesUp(context.getInputBlock());  // USA ISTANZA ATTIVA

        const size_t numOversampledChannels = oversampledBlock.getNumChannels();
        const size_t numOversampledSamples = oversampledBlock.getNumSamples();

        // ═══════════════════════════════════════════════════════
        // PROCESSING LOOP (oversampled)
        // ═══════════════════════════════════════════════════════
        for (size_t sample = 0; sample < numOversampledSamples; ++sample)
        {
            // Map sample index to native rate envelope
            size_t nativeIndex = sample / activeFactor;  // USA FATTORE CORRETTO
            if (nativeIndex >= envelopeBuffer.getNumSamples())
                nativeIndex = envelopeBuffer.getNumSamples() - 1;

            float env = envData[nativeIndex] + 1.0f; // envelope modulation (1-2)
            float currentWidth = stereoWidth.getNextValue();
            float driveValue = drive.getNextValue();

            // Stereo bias
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Process each channel
            for (size_t ch = 0; ch < numOversampledChannels; ++ch)
            {
                auto dataPtr = oversampledBlock.getChannelPointer(ch);
                float input = dataPtr[sample];

                // 1. Apply drive
                float driven = input * driveValue;

                // 2. Add stereo bias
                driven += (ch == 0) ? biasL : biasR;

                // 3. Modulate with envelope
                driven *= env;

                // 4. Apply waveshaping (type-dependent)
                dataPtr[sample] = applyWaveshaping(driven);
            }
        }

        // ═══════════════════════════════════════════════════════
        // OVERSAMPLING DOWN
        // ═══════════════════════════════════════════════════════
        activeOversampler->processSamplesDown(context.getOutputBlock());

        // ═══════════════════════════════════════════════════════
        // DC BLOCKER + GAIN COMP (native rate)
        // ═══════════════════════════════════════════════════════
        auto bufferData = buffer.getArrayOfWritePointers();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                bufferData[ch][i] = dcBlocker[ch].processSample(bufferData[ch][i]);
                bufferData[ch][i] *= 0.5f; // gain compensation
            }
        }
    }

private:
    // ═══════════════════════════════════════════════════════════
    // WAVESHAPING FUNCTIONS (TYPE-SPECIFIC)
    // ═══════════════════════════════════════════════════════════
    float applyWaveshaping(float x)
    {
        switch (currentType)
        {
        case WaveshapeType::SineFold:
            return sineFold(x);

        case WaveshapeType::Foldback:
            return foldback(x);

        case WaveshapeType::Triangle:
            return triangleWavefolder(x);

        case WaveshapeType::Chebyshev:
            return chebyshevPoly(x);

        default:
            return x;
        }
    }

    // B: Sine Wavefolder (smooth, musical)
    static float sineFold(float x)
    {
        return std::sin(juce::MathConstants<float>::twoPi * x);
    }

    // ═══════════════════════════════════════════════════════════════
// D: FOLDBACK WAVEFOLDER (Serge-style)
// Formula classica:
//   - Se |x| <= threshold: return x (lineare)
//   - Se |x| > threshold: rifletti il segnale attorno alla soglia
// 
// Implementazione: 
//   foldback(x, thresh) = thresh - |x - thresh| se x > thresh
//                       = -thresh + |x + thresh| se x < -thresh
//                       = x altrimenti
// ═══════════════════════════════════════════════════════════════
    static float foldback(float x)
    {
        constexpr float threshold = 0.25f; // Soglia di folding
        constexpr float gainComp = 1.0f / threshold; 

        // Hard folding (riflessione geometrica)
        while (x > threshold || x < -threshold)
        {
            if (x > threshold)
                x = threshold - (x - threshold);
            if (x < -threshold)
                x = -threshold + (-threshold - x);
        }

        return x * gainComp; // <-- GAIN COMPENSATION
    }
    
    // ═══════════════════════════════════════════════════════════════
// C: TRIANGLE WAVEFOLDER (Stanford CCRMA)
// Paper: "Waveshaping Synthesis" - Julius O. Smith III
// Formula: y(x) = 4 * |( (x/period) - floor((x/period) + 0.5) )| - 1
// 
// Simplified per period = 2:
//   y(x) = 4 * |x - 2*floor((x+1)/2)| - 1
// ═══════════════════════════════════════════════════════════════
    static float triangleWavefolder(float x)
    {
        // Formula Stanford (period = 2, range [-1, 1])
        constexpr float period = 1.0f;

        // Calcola la fase normalizzata
        float phase = x / period;

        // Applica la formula triangolare
        float folded = 4.0f * std::abs(phase - std::floor(phase + 0.5f)) - 1.0f;

        return folded;
    }

    // ═══════════════════════════════════════════════════════════════
    // A: CHEBYSHEV POLYNOMIAL (3rd order)
    // Formula: T3(x) = 4x³ - 3x
    // Range ottimale input: [-1, 1]
    // Produce principalmente 3rd harmonic (ottave + quinta)
    // ═══════════════════════════════════════════════════════════════
    static float chebyshevPoly(float x)
    {
        // Soft clipping prima di applicare il polinomio per stabilità
        x = std::tanh(x);

        // Chebyshev T3(x) = 4x³ - 3x (genera 3rd harmonic)
        return 4.0f * x * x * x - 3.0f * x;
    }

    // ═══════════════════════════════════════════════════════════
    // FIX: OVERSAMPLER INITIALIZATION (DUAL INSTANCES)
    // ═══════════════════════════════════════════════════════════
    void initOversamplers(int samplesPerBlock)
    {
        // NUOVO: Oversampler bypass (1x, mantiene latenza coerente)
        oversamplerBypass = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // stereo
            0, // 2^0 = 1x (nessun oversampling)
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerBypass->initProcessing(static_cast<size_t>(samplesPerBlock));

        // NUOVO: Oversampler high quality (4x+)
        oversamplingFactorHigh = static_cast<int>(TARGET_SAMPLING_RATE / originalSampleRate);
        oversamplingFactorHigh = juce::jmin(16, juce::jmax(1, oversamplingFactorHigh));

        oversamplerHigh = std::make_unique<juce::dsp::Oversampling<float>>(
            2,
            static_cast<size_t>(std::log2(oversamplingFactorHigh)),  // FATTORE ALTO
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerHigh->initProcessing(static_cast<size_t>(samplesPerBlock));
    }

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> drive;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> stereoWidth;
    juce::dsp::IIR::Filter<float> dcBlocker[2];

    WaveshapeType currentType;
    bool oversampling;

    double originalSampleRate = 0.0;
    int maxSamplesPerBlock = 0;
    int oversamplingFactorHigh = 1;  // FIX: fattore per high quality

    // FIX: DUE ISTANZE SEPARATE
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerBypass;  // (1x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerHigh;    // (4x+)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperCore)
};
