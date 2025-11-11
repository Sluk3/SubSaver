#pragma once

#include <JuceHeader.h>

class EnvelopeFollower
{
public:
    EnvelopeFollower(float defaultAmount = 1.0f)
        : amount(defaultAmount),
        envelope(0.0f),
        sampleRate(44100.0)
    {
        amount.setCurrentAndTargetValue(defaultAmount);
    }

    void prepareToPlay(double sr)
    {
        sampleRate = sr;
        envelope = 0.0f;
        amount.reset(sr, 0.03);

        // Lowpass filter per envelope smoothing (20Hz come in PD)
        lpCoeff = 1.0f - std::exp(-juce::MathConstants<float>::twoPi * 20.0f / static_cast<float>(sr));
    }

    void setModAmount(float amountValue)
    {
        amount.setTargetValue(juce::jlimit(0.0f, 1.0f, amountValue));
    }

    // Genera envelope buffer (già scalato per env_amount)
    void processBlock(const juce::AudioBuffer<float>& inputBuffer, juce::AudioBuffer<double>& envelopeBuffer)
    {
        const int numChannels = inputBuffer.getNumChannels();
        const int numSamples = inputBuffer.getNumSamples();

        // Envelope mono
        envelopeBuffer.setSize(1, numSamples, false, false, true);
        auto envData = envelopeBuffer.getWritePointer(0);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // 1. Full-wave rectifier: somma L+R in valore assoluto
            float sum = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                sum += std::abs(inputBuffer.getSample(ch, sample));
            }

            // 2. Lowpass filter a 20Hz (one-pole smoothing)
            envelope += lpCoeff * (sum - envelope);

            // 3. Scala per env_amount e output
            float currentAmount = amount.getNextValue();
            envData[sample] = envelope * currentAmount;
        }
    }

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amount;
    float envelope;
    float lpCoeff;
    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower);
};
