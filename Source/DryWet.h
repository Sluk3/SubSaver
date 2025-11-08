#pragma once

#include <JuceHeader.h>

class DryWet
{
public:
    DryWet(float defaultDry = 1.0f, float defaultWet = 1.0f, int defaultDelaySamples = 0)
        : dryLevel(defaultDry),
        wetLevel(defaultWet),
        delaySamples(defaultDelaySamples)
    {
    }

    ~DryWet() {}

    void prepareToPlay(double /*sampleRate*/, int maxNumSamples)
    {
        drySignal.setSize(2, maxNumSamples);
        drySignal.clear();
        delayBuffer.setSize(2, maxNumSamples + delaySamples);
        delayBuffer.clear();
    }

    void releaseResources()
    {
        drySignal.setSize(0, 0);
        delayBuffer.setSize(0, 0);
    }

    //void processDW(juce::AudioBuffer<float>& wetBuffer, const juce::AudioBuffer<float>& inputBuffer)
    //{
    //    const int numChannels = wetBuffer.getNumChannels();
    //    const int numSamples = wetBuffer.getNumSamples();

    //    // copia dry prima della VCA
    //    for (int ch = 0; ch < numChannels; ++ch)
    //        drySignal.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);

    //    // delay compensation sul dry (solo se delaySamples > 0)
    //    for (int ch = 0; ch < numChannels; ++ch)
    //    {
    //        if (delaySamples > 0)
    //        {
    //            delayBuffer.copyFrom(ch, 0, drySignal, ch, 0, numSamples);
    //            drySignal.copyFrom(ch, 0, delayBuffer, ch, delaySamples, numSamples - delaySamples);
    //        }
    //        drySignal.applyGain(ch, 0, numSamples, dryLevel);
    //    }

    //    // Applica gain wet, somma dry e wet
    //    wetBuffer.applyGain(wetLevel);

    //    for (int ch = 0; ch < numChannels; ++ch)
    //        wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples);
    //}

    void copyDrySignal(const juce::AudioBuffer<float>& inputBuffer)
    {
        const int numChannels = inputBuffer.getNumChannels();
        const int numSamples = inputBuffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
            drySignal.copyFrom(ch, 0, inputBuffer, ch, 0, numSamples);
    }

    void mergeDryAndWet(juce::AudioBuffer<float>& wetBuffer)
    {
        const int numChannels = wetBuffer.getNumChannels();
        const int numSamples = wetBuffer.getNumSamples();

        // delay compensation sul dry (solo se delaySamples > 0)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            if (delaySamples > 0)
            {
				delayBuffer.copyFrom(ch, 0, drySignal, ch, 0, numSamples); //sbagliato, cambia con delayline
                drySignal.copyFrom(ch, 0, delayBuffer, ch, delaySamples, numSamples - delaySamples);
            }
            drySignal.applyGain(ch, 0, numSamples, dryLevel);
        }

        // Applica gain wet, somma dry e wet
        wetBuffer.applyGain(wetLevel);

        for (int ch = 0; ch < numChannels; ++ch)
            wetBuffer.addFrom(ch, 0, drySignal, ch, 0, numSamples);
    }

    void setDryLevel(float value) { dryLevel = value; }
    void setWetLevel(float value) { wetLevel = value; }
    void setDelaySamples(int samples) { delaySamples = samples; }

private:
    float dryLevel;
    float wetLevel;
    int delaySamples;

    juce::AudioBuffer<float> drySignal;
    juce::AudioBuffer<float> delayBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DryWet);
};
