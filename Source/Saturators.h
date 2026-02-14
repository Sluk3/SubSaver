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
    WaveshaperCore(double defaultDrive = Parameters::defaultDrive, double defaultStereoWidth = Parameters::defaultStereoWidth, bool defaultOversampling = Parameters::defaultOversampling)
        : drive(defaultDrive),
        stereoWidth(defaultStereoWidth),
        oversampling(defaultOversampling),
        morphValue(0.0f)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
        morphValue.setCurrentAndTargetValue(0.0f);
    }

    // ═══════════════════════════════════════════════════════════
    // SETUP & CONFIGURATION
    // ═══════════════════════════════════════════════════════════
    void prepareToPlay(double sampleRate, int samplesPerBlock, int numCh)
    {
        drive.reset(sampleRate, 0.03);
        stereoWidth.reset(sampleRate, 0.03);
        morphValue.reset(sampleRate, 0.25);  // FIX: 250ms smoothing (era 0.03)

        // DC blocker (HPF 5-7.5Hz)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 7.5);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = coeffs;
            dcBlocker[ch].reset();
        }

        maxSamplesPerBlock = samplesPerBlock;
        originalSampleRate = sampleRate;
        
        initOversamplers(samplesPerBlock);
    }

    
    void setMorphValue(float value)
    {
        morphValue.setTargetValue(juce::jlimit(0.0f, 3.0f, value));
    }
    void setOversampling(bool shouldOversample)
    {
        oversampling = shouldOversample;
    }

    void setDrive(double value) { drive.setTargetValue(value); }
    void setStereoWidth(float width) { stereoWidth.setTargetValue(width); }

    int getLatencySamples() const noexcept
    {
        if (oversampling && oversamplerHigh)
            return static_cast<int>(oversamplerHigh->getLatencyInSamples());
        else if (oversamplerBypass)
            return static_cast<int>(oversamplerBypass->getLatencyInSamples());
        return 0;
    }

    // ═══════════════════════════════════════════════════════════
    // PROCESS BLOCK
    // ═══════════════════════════════════════════════════════════
    void processBlock(juce::AudioBuffer<float>& buffer,
        const juce::AudioBuffer<double>& envelopeBuffer)
    {
        auto* activeOversampler = oversampling ? oversamplerHigh.get() : oversamplerBypass.get();
        const int activeFactor = oversampling ? oversamplingFactorHigh : 1;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        auto envData = envelopeBuffer.getReadPointer(0);

        // ═══════════════════════════════════════════════════════
        // FIX: Determina se il morph è in fase di smoothing
        // Se stabile → campiona una volta (elimina DC artifacts)
        // Se in transizione → aggiorna ad ogni sample (smooth)
        // ═══════════════════════════════════════════════════════
        const bool morphIsSmoothing = morphValue.isSmoothing();
        float currentMorphValue = morphIsSmoothing ? 0.0f : morphValue.getCurrentValue();

        // ═══════════════════════════════════════════════════════
        // OVERSAMPLING UP
        // ═══════════════════════════════════════════════════════
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        auto oversampledBlock = activeOversampler->processSamplesUp(context.getInputBlock());

        const size_t numOversampledChannels = oversampledBlock.getNumChannels();
        const size_t numOversampledSamples = oversampledBlock.getNumSamples();

        // ═══════════════════════════════════════════════════════
        // PROCESSING LOOP (oversampled)
        // ═══════════════════════════════════════════════════════
        for (size_t sample = 0; sample < numOversampledSamples; ++sample)
        {
            // FIX: Aggiorna morphValue solo se in smoothing
            if (morphIsSmoothing)
                currentMorphValue = morphValue.getNextValue();

            // Map sample index to native rate envelope
            size_t nativeIndex = sample / activeFactor;
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

                // 4. Apply waveshaping (passa morph come parametro)
                dataPtr[sample] = applyWaveshaping(driven, currentMorphValue);
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
    float applyWaveshaping(float x, float morph)  // FIX: riceve morph come parametro
    {
        // Calcola tutte e 4 le funzioni
        float shape0 = chebyshevPoly(x);      // 0.0
        float shape1 = sineFold(x);           // 1.0
        float shape2 = triangleWavefolder(x); // 2.0
        float shape3 = foldback(x);           // 3.0

        // Determina quale coppia di funzioni interpolare
        if (morph < 1.0f)
        {
            // Morph tra Chebyshev (0) e SineFold (1)
            float blend = morph;
            return shape0 * (1.0f - blend) + shape1 * blend;
        }
        else if (morph < 2.0f)
        {
            // Morph tra SineFold (1) e Triangle (2)
            float blend = morph - 1.0f;
            return shape1 * (1.0f - blend) + shape2 * blend;
        }
        else
        {
            // Morph tra Triangle (2) e Foldback (3)
            float blend = morph - 2.0f;
            return shape2 * (1.0f - blend) + shape3 * blend;
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

        return x * gainComp;
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
    // OVERSAMPLER INITIALIZATION (DUAL INSTANCES)
    // ═══════════════════════════════════════════════════════════
    void initOversamplers(int samplesPerBlock)
    {
        // Oversampler bypass (1x, mantiene latenza coerente)
        oversamplerBypass = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // stereo
            0, // 2^0 = 1x (nessun oversampling)
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerBypass->initProcessing(static_cast<size_t>(samplesPerBlock));

        // Oversampler high quality (4x+)
        oversamplingFactorHigh = static_cast<int>(TARGET_SAMPLING_RATE / originalSampleRate);
        oversamplingFactorHigh = juce::jmin(16, juce::jmax(1, oversamplingFactorHigh));

        oversamplerHigh = std::make_unique<juce::dsp::Oversampling<float>>(
            2,
            static_cast<size_t>(std::log2(oversamplingFactorHigh)),
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerHigh->initProcessing(static_cast<size_t>(samplesPerBlock));
    }

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> drive;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> stereoWidth;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> morphValue;
    juce::dsp::IIR::Filter<float> dcBlocker[2];

    WaveshapeType currentType;
    bool oversampling;

    double originalSampleRate = 0.0;
    int maxSamplesPerBlock = 0;
    int oversamplingFactorHigh = 1;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerBypass;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerHigh;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperCore)
};
