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
        // Imposta DC blocker (highpass 5Hz, come hip~ 5 in PD)
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 10.0);
        for (int ch = 0; ch < 2; ++ch)
        {
            dcBlocker[ch].coefficients = coeffs;
            dcBlocker[ch].reset();
        }
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
    // Procesing: apply foldback per canale wsddith drive and threshold
    void processBlock(juce::AudioBuffer<float>& buffer,
        const juce::AudioBuffer<double>& envelopeBuffer)
    {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
		auto driveValue = 1.0f;
        auto bufferData = buffer.getArrayOfWritePointers();
        auto envData = envelopeBuffer.getReadPointer(0);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            float env = envData[sample]+1;
            float currentWidth = stereoWidth.getNextValue();
			driveValue = drive.getNextValue();
            // Calcola i bias per L e R
            float biasL = currentWidth * (-0.5f);
            float biasR = currentWidth * (+0.5f);

            // Processa ogni canale
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float bias = (ch == 0) ? biasL : biasR;

                // 1. input  drive
                float driven = bufferData[ch][sample] * driveValue;

                // 2. + bias
                driven += bias;

                // 3.  envelope_mod
                driven *= env;

                // 4. sin~ (o altra waveshaping function)
                bufferData[ch][sample] = sineFold(driven, threshold);

                // 5. DC block (equivalente a hip~ 5)
                bufferData[ch][sample] = dcBlocker[ch].processSample(bufferData[ch][sample]);

                // 6. Gain compensation (*~ 0.5)
                bufferData[ch][sample] *= 0.5f;
            }
        }
    }


    static float sineFold(float x, float drive)
    {
        // Modula il segnale con una funzione sinusoidale
        // drive controlla quante volte il segnale viene "piegato"
        return std::sin(juce::MathConstants<float>::twoPi * x * drive);
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;
    juce::dsp::IIR::Filter<float> dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SineFoldSaturator);
};















class TriFoldSaturator
{
public:
    TriFoldSaturator(double defaultDrive = Parameters::defaultDrive,
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

                // Applica: drive dulato + bias, poi foldback
                float driven = bufferData[ch][sample] * modulatedDrive + bias;
                bufferData[ch][sample] = triangleWavefolder(driven, threshold);
            }
        }
    }

    static float triangleWavefolder(float x, float drive)
    {
        // Normalizza con drive per controllare il folding
        float normalized = x * drive;

        // Triangle wave formula dalla documentazione Stanford
        float period = 1.0f / drive;
        float phase = normalized + period / 4.0f;

        return 4.0f * std::abs((phase / period) - std::floor((phase / period) + 0.5f)) - 1.0f;
    }

private:
    SmoothedValue<double, ValueSmoothingTypes::Linear> drive;
    SmoothedValue<double, ValueSmoothingTypes::Linear> stereoWidth;
    float threshold;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriFoldSaturator);
};