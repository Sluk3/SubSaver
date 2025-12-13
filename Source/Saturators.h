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
        resetOversampler();
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
        if (oversampling == shouldOversample)
            return;
        oversampling = shouldOversample;
        needsOversamplerReset = true;
    }

    void setDrive(double value) { drive.setTargetValue(value); }
    void setStereoWidth(float width) { stereoWidth.setTargetValue(width); }

    int getLatencySamples() const noexcept
    {
        if (oversampler)
            return static_cast<int>(oversampler->getLatencyInSamples());
        return 0;
    }

    // ═══════════════════════════════════════════════════════════
    // PROCESS BLOCK
    // ═══════════════════════════════════════════════════════════
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (needsOversamplerReset)
        {
            resetOversampler();
            needsOversamplerReset = false;
        }

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        // ═══════════════════════════════════════════════════════
        // OVERSAMPLING UP
        // ═══════════════════════════════════════════════════════
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        auto oversampledBlock = oversampler->processSamplesUp(context.getInputBlock());

        const size_t numOversampledChannels = oversampledBlock.getNumChannels();
        const size_t numOversampledSamples = oversampledBlock.getNumSamples();

        // ═══════════════════════════════════════════════════════
        // PROCESSING LOOP (oversampled)
        // ═══════════════════════════════════════════════════════
        for (size_t sample = 0; sample < numOversampledSamples; ++sample)
        {
            // Map sample index to native rate envelope
            size_t nativeIndex = sample / oversamplingFactor;
            
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


                // 3. Apply waveshaping
                dataPtr[sample] = applyWaveshaping(driven);
            }
        }

        // ═══════════════════════════════════════════════════════
        // OVERSAMPLING DOWN
        // ═══════════════════════════════════════════════════════
        oversampler->processSamplesDown(context.getOutputBlock());

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
       return sineFold(x);
    }

    // B: Sine Wavefolder (smooth, musical)
    static float sineFold(float x)
    {
        return std::sin(juce::MathConstants<float>::twoPi * x);
    }


    // ═══════════════════════════════════════════════════════════
    // OVERSAMPLER MANAGEMENT
    // ═══════════════════════════════════════════════════════════
    void resetOversampler()
    {
        oversamplingFactor = oversampling
            ? static_cast<int>(TARGET_SAMPLING_RATE / originalSampleRate)
            : 1;
        oversamplingFactor = juce::jmin(16, juce::jmax(1, oversamplingFactor));

        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // stereo
            static_cast<size_t>(std::log2(oversamplingFactor)),
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true, // normalisedFilters
            true  // integerLatency
        );

        if (oversampler)
            oversampler->initProcessing(static_cast<size_t>(maxSamplesPerBlock));
    }

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> drive;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> stereoWidth;
    juce::dsp::IIR::Filter<float> dcBlocker[2];

    WaveshapeType currentType;
    bool oversampling;
    bool needsOversamplerReset = false;

    double originalSampleRate = 0.0;
    int maxSamplesPerBlock = 0;
    int oversamplingFactor = 1;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperCore)
};
