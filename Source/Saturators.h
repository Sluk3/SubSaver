#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
class FoldbackSaturator
{
public:
    FoldbackSaturator(double defaultDrive = Parameters::defaultDrive,
        float defaultThresh = 0.8f,
        double defaultStereoWidth = Parameters::defaultStereoWidth)
        : drive(defaultDrive),
        threshold(defaultThresh),
        stereoWidth(defaultStereoWidth)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
        stereoWidth.reset(sampleRate, 0.03);
    }

    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setThreshold(float thresh)
    {
        threshold = thresh;
    }
    void setStereoWidth(float width)
    {
        stereoWidth.setTargetValue(width);
    }
    // Procesing: apply foldback per canale with drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<double>& modulatedDriveBuffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        auto bufferData = buffer.getArrayOfWritePointers();
        auto modDriveData = modulatedDriveBuffer.getArrayOfReadPointers();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            
            double modulatedDrive = modDriveData[0][sample];
            float currentWidth = stereoWidth.getNextValue();

            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // Applica: drive modulato + bias, poi foldback
                float driven = bufferData[ch][sample] * modulatedDrive + bias;
                bufferData[ch][sample] = foldback(driven, threshold);
            }
        }
    }

    static float foldback(float x, float thresh)
    {
		// 1. Controllo della soglia -> Se il segnale supera la soglia applica la distorsione foldback, altrimenti ritorna il segnale inalterato
        if (std::abs(x) > thresh)
            // 2. Applicazione della distorsione foldback
            return std::abs(std::fmod(x - thresh, thresh * 4)) - thresh;
		// 3. return del segnale inalterato
        return x;
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldbackSaturator);
};
#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
class SineFoldSaturator
{
public:
    SineFoldSaturator(double defaultDrive = Parameters::defaultDrive,
        float defaultThresh = 0.8f,
        double defaultStereoWidth = Parameters::defaultStereoWidth)
        : drive(defaultDrive),
        threshold(defaultThresh),
        stereoWidth(defaultStereoWidth)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
        stereoWidth.setCurrentAndTargetValue(defaultStereoWidth);
    }

    void prepareToPlay(double sampleRate)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
        stereoWidth.reset(sampleRate, 0.03);
    }

    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setThreshold(float thresh)
    {
        threshold = thresh;
    }
    void setStereoWidth(float width)
    {
        stereoWidth.setTargetValue(width);
    }
    // Procesing: apply foldback per canale with drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<double>& modulatedDriveBuffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        auto bufferData = buffer.getArrayOfWritePointers();
        auto modDriveData = modulatedDriveBuffer.getArrayOfReadPointers();

        for (int sample = 0; sample < numSamples; ++sample)
        {

            double modulatedDrive = modDriveData[0][sample];
            float currentWidth = stereoWidth.getNextValue();

            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // Applica: drive modulato + bias, poi foldback
                float driven = bufferData[ch][sample] * modulatedDrive + bias;
                bufferData[ch][sample] = sineFold(driven, threshold);
            }
        }
    }

    static float sineFold(float x, float thresh)
    {
        // 1. Controllo della soglia
        if (std::abs(x) > thresh)
            // 2. Applicazione sine folding: più musicale e smooth
            return thresh * std::sin((x / thresh) * juce::MathConstants<float>::pi);
        // 3. Return del segnale inalterato
        return x;
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SineFoldSaturator);
};
