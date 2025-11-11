#pragma once

#include <JuceHeader.h>

class EnvelopeFollower
{
public:
    EnvelopeFollower(float defaultAttack = 0.01f, float defaultRelease = 0.1f)
        : attackTime(defaultAttack),
        releaseTime(defaultRelease),
        envelope(0.0f),
        sampleRate(44100.0)
    {
    }

    void prepareToPlay(double sr)
    {
        sampleRate = sr;
        envelope = 0.0f;
        updateCoefficients();
    }

    void setAttack(float attackSeconds)
    {
        attackTime = juce::jlimit(0.001f, 0.1f, attackSeconds);
        updateCoefficients();
    }

    void setRelease(float releaseSeconds)
    {
        releaseTime = juce::jlimit(0.01f, 2.0f, releaseSeconds);
        updateCoefficients();
    }

    // Genera l'envelope (0-1) in un buffer
    void processBlock(const juce::AudioBuffer<float>& inputBuffer,
        juce::AudioBuffer<double>& envelopeBuffer)
    {
        const int numChannels = inputBuffer.getNumChannels();
        const int numSamples = inputBuffer.getNumSamples();

        // Buffer mono per l'envelope
        envelopeBuffer.setSize(1, numSamples, false, false, true);
        auto envData = envelopeBuffer.getWritePointer(0);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // 1. Full-wave rectifier: somma L+R in valore assoluto
            double sum = 0.0;
            for (int ch = 0; ch < numChannels; ++ch)
            {
                sum += std::abs(inputBuffer.getSample(ch, sample));
            }
            double rectified = sum / numChannels;

            // 2. Envelope follower (attack/release)
            if (rectified > envelope)
                envelope += attackCoeff * (rectified - envelope);
            else
                envelope += releaseCoeff * (rectified - envelope);

            // 3. Output normalizzato 0-1
            envData[sample] = envelope;
        }
    }

private:
    void updateCoefficients()
    {
        attackCoeff = 1.0 - std::exp(-1.0 / (attackTime * sampleRate));
        releaseCoeff = 1.0 - std::exp(-1.0 / (releaseTime * sampleRate));
    }

    float attackTime;
    float releaseTime;
    double envelope;
    double attackCoeff;
    double releaseCoeff;
    double sampleRate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFollower);
};


// ParameterModulation.h - la tua classe, con piccola modifica per chiarezza
#pragma once

#include <JuceHeader.h>

class ParameterModulation
{
public:
    ParameterModulation(const double defaultParameter, const double defaultModAmount)
    {
        parameter.setCurrentAndTargetValue(defaultParameter);
        modAmount.setCurrentAndTargetValue(defaultModAmount);
    }

    ~ParameterModulation() = default;

    void prepareToPlay(double sampleRate)
    {
        parameter.reset(sampleRate, 0.02);
        modAmount.reset(sampleRate, 0.02);
    }

    // Prende in input il buffer di modulazione (LFO o Envelope) già normalizzato tra -1 e +1
    // e lo scala/applica al parametro
    void processBlock(juce::AudioBuffer<double>& buffer, const int numSamples)
    {
        const auto numCh = buffer.getNumChannels();
        auto data = buffer.getArrayOfWritePointers();

        // 1. Riscala modulazione tra 0 e 1 (se arriva tra -1 e +1)
        for (int ch = 0; ch < numCh; ++ch)
        {
            juce::FloatVectorOperations::add(data[ch], 1.0, numSamples);
            juce::FloatVectorOperations::multiply(data[ch], 0.5, numSamples); // Ora è 0-1
        }

        // 2. Moltiplica per modAmount
        for (int ch = 0; ch < numCh; ++ch)
        {
            juce::FloatVectorOperations::multiply(data[ch], modAmount.getCurrentValue(), numSamples);
        }

        // 3. Aggiungi il valore base del parametro
        if (parameter.isSmoothing())
        {
            for (int sample = 0; sample < numSamples; ++sample)
            {
                double baseParam = (sample == 0) ? parameter.getNextValue() : parameter.getCurrentValue();
                for (int ch = 0; ch < numCh; ++ch)
                {
                    data[ch][sample] += baseParam;
                }
            }
        }
        else
        {
            for (int ch = 0; ch < numCh; ++ch)
            {
                juce::FloatVectorOperations::add(data[ch], parameter.getCurrentValue(), numSamples);
            }
        }
    }

    void setModAmount(double newValue)
    {
        modAmount.setTargetValue(newValue);
    }

    void setParameter(const double newValue)
    {
        parameter.setTargetValue(newValue);
    }

private:
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> modAmount;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> parameter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParameterModulation);
};
