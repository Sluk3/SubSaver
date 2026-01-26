#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"

#define TARGET_SAMPLING_RATE 192000.0

class WaveshaperCore
{
public:
    WaveshaperCore(double defaultDrive = Parameters::defaultDrive, double defaultStereoWidth = Parameters::defaultStereoWidth, bool defaultOversampling = Parameters::defaultOversampling)
        : drive(defaultDrive),
        stereoWidth(defaultStereoWidth),
        oversampling(defaultOversampling)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock, int numCh)
    {
        drive.reset(sampleRate, 0.03);
        stereoWidth.reset(sampleRate, 0.03);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 7.5);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = coeffs;
            dcBlocker[ch].reset();
        }

        maxSamplesPerBlock = samplesPerBlock;
        originalSampleRate = sampleRate;

        // NUOVO: Inizializza ENTRAMBI gli oversampler
        initOversamplers(samplesPerBlock);  // NUOVA FUNZIONE
    }


    void setOversampling(bool shouldOversample)
    {
        oversampling = shouldOversample;
        // NESSUN RESET - solo switch tra istanze già pronte
    }

    void setDrive(double value) { drive.setTargetValue(value); }
    void setStereoWidth(float width) { stereoWidth.setTargetValue(width); }

    // NUOVO CODICE:
    int getLatencySamples() const noexcept
    {
        if (oversampling && oversamplerHigh)  //   SELEZIONA L'ISTANZA CORRETTA
            return static_cast<int>(oversamplerHigh->getLatencyInSamples());
        else if (oversamplerBypass)  //   ISTANZA BYPASS
            return static_cast<int>(oversamplerBypass->getLatencyInSamples());
        return 0;
    }

    // PROCESS BLOCK
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        //   NESSUN RESET RUNTIME

        //   SELEZIONE ISTANZA CORRETTA
        auto* activeOversampler = oversampling ? oversamplerHigh.get() : oversamplerBypass.get();
        const int activeFactor = oversampling ? oversamplingFactorHigh : 1;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        //auto envData = envelopeBuffer.getReadPointer(0);
		// OVERSAMPLING UP ------------------------------------------------
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        auto oversampledBlock = activeOversampler->processSamplesUp(context.getInputBlock());  //   USA ISTANZA ATTIVA

        const size_t numOversampledChannels = oversampledBlock.getNumChannels();
        const size_t numOversampledSamples = oversampledBlock.getNumSamples();

        // PROCESSING LOOP (oversampled)
        for (size_t sample = 0; sample < numOversampledSamples; ++sample)
        {
            // Map sample index to native rate envelope
            size_t nativeIndex = sample / activeFactor;  //   USA FATTORE CORRETTO
            
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

        // OVERSAMPLING DOWN ------------------------------------------------
        activeOversampler->processSamplesDown(context.getOutputBlock());

        // DC BLOCKER + GAIN COMP (native rate)
        auto bufferData = buffer.getArrayOfWritePointers();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                bufferData[ch][i] = dcBlocker[ch].processSample(bufferData[ch][i]);
                bufferData[ch][i] *= 0.7f; // gain compensation
            }
        }
    }

private:
    
    float applyWaveshaping(float x)
    {
       return sineFold(x);
    }

    // Sine Wavefolder 
    static float sineFold(float x)
    {
        return std::sin(juce::MathConstants<float>::twoPi * x);
    }


    void initOversamplers(int samplesPerBlock)
    {
        // NUOVO: Oversampler bypass (1x, mantiene latenza coerente)
        oversamplerBypass = std::make_unique<juce::dsp::Oversampling<float>>(
            2, // stereo
            0, //  2^0 = 1x (nessun oversampling)
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerBypass->initProcessing(static_cast<size_t>(samplesPerBlock));

        //   NUOVO: Oversampler high quality (4x+)
        oversamplingFactorHigh = static_cast<int>(TARGET_SAMPLING_RATE / originalSampleRate);
        oversamplingFactorHigh = juce::jmin(16, juce::jmax(1, oversamplingFactorHigh));

        oversamplerHigh = std::make_unique<juce::dsp::Oversampling<float>>(
            2,
            static_cast<size_t>(std::log2(oversamplingFactorHigh)),  //   FATTORE ALTO
            juce::dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR,
            true,
            true
        );
        oversamplerHigh->initProcessing(static_cast<size_t>(samplesPerBlock));
    }

    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> drive;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> stereoWidth;
    juce::dsp::IIR::Filter<float> dcBlocker[2];

    bool oversampling;

    double originalSampleRate = 0.0;
    int maxSamplesPerBlock = 0;
    int oversamplingFactorHigh = 1; 

    // DUE ISTANZE SEPARATE
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerBypass;  //  (1x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversamplerHigh;    //  (4x+)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperCore)
};
