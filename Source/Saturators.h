#pragma once

#include <JuceHeader.h>
#include "PluginParameters.h"
class FoldbackSaturator
{
public:
    FoldbackSaturator(double defaultDrive = Parameters::defaultDrive, float defaultThresh = 0.8f)
        : drive(defaultDrive),
        threshold(defaultThresh)
    {
        drive.setCurrentAndTargetValue(defaultDrive);
    }

    void prepareToPlay(double sampleRate)
    {
        drive.reset(sampleRate, 0.03); // Smoothing attack/release veloce
    }

    void setDrive(double value)
    {
        drive.setTargetValue(value);
    }
    void setThreshold(float thresh)
    {
        threshold = thresh;
    }

    // Procesing: apply foldback per canale with drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        auto bufferData = buffer.getArrayOfWritePointers();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            double currentDrive = drive.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                bufferData[ch][sample] = foldback(bufferData[ch][sample] * currentDrive, threshold);
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
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FoldbackSaturator);
};
