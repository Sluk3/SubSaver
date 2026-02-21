#pragma once
#include <JuceHeader.h>
#include "PluginParameters.h"
#include "Saturators.h"  

class WaveformDisplay : public juce::Component,
    public juce::AudioProcessorValueTreeState::Listener,
    private juce::AsyncUpdater
{
public:
    explicit WaveformDisplay(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
    {
        apvts.addParameterListener(Parameters::nameMorph, this);
        apvts.addParameterListener(Parameters::nameDrive, this);

        if (auto* p = apvts.getRawParameterValue(Parameters::nameMorph))
            morph.store(p->load());
        if (auto* p = apvts.getRawParameterValue(Parameters::nameDrive))
            drive.store(p->load());
    }

    ~WaveformDisplay() override
    {
        cancelPendingUpdate();
        apvts.removeParameterListener(Parameters::nameMorph, this);
        apvts.removeParameterListener(Parameters::nameDrive, this);
    }

    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        if (parameterID == Parameters::nameMorph)
            morph.store(newValue);
        else if (parameterID == Parameters::nameDrive)
            drive.store(newValue);

        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override { repaint(); }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(3.0f);
        const float cy = bounds.getCentreY();
        const float amplitude = bounds.getHeight() * 0.40f;

        // ── Background ──────────────────────────────────────────────────
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.drawHorizontalLine(static_cast<int>(cy),
            bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

        const int   numPoints = 300;
        const float currentMorph = morph.load();
        const float currentDrive = drive.load();

        // ── Sine di riferimento (tratteggiata, bianca) ─────────────────
        {
            juce::Path refPath;
            for (int i = 0; i < numPoints; ++i)
            {
                const float t = float(i) / float(numPoints - 1);
                const float px = bounds.getX() + t * bounds.getWidth();
                const float py = cy - std::sin(t * juce::MathConstants<float>::twoPi * 1.5f) * amplitude;
                (i == 0) ? refPath.startNewSubPath(px, py) : refPath.lineTo(px, py);
            }
            juce::Path dashed;
            float dashLengths[] = { 4.0f, 5.0f };
            juce::PathStrokeType(1.0f).createDashedStroke(dashed, refPath, dashLengths, 2);
            g.setColour(juce::Colours::white.withAlpha(0.30f));
            g.fillPath(dashed);
        }

        // ── Sine distorta (rossa) ──────────────────────────────────────
        {
            juce::Path distPath;
            for (int i = 0; i < numPoints; ++i)
            {
                const float t = float(i) / float(numPoints - 1);
                const float px = bounds.getX() + t * bounds.getWidth();
                const float input = std::sin(t * juce::MathConstants<float>::twoPi * 1.5f);
                const float driven = input * currentDrive;

                // Usa direttamente WaveshaperCore — nessuna duplicazione
                const float output = waveshaper.applyWaveshaping(driven, currentMorph);

                const float py = cy - juce::jlimit(-1.0f, 1.0f, output) * amplitude;
                (i == 0) ? distPath.startNewSubPath(px, py) : distPath.lineTo(px, py);
            }
            g.setColour(juce::Colour(0xffff3333));
            g.strokePath(distPath, juce::PathStrokeType(
                1.0f,
                juce::PathStrokeType::curved,
                juce::PathStrokeType::rounded));
        }
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    WaveshaperCore waveshaper;  // costruttore leggero, prepareToPlay() mai chiamato
    std::atomic<float> morph{ Parameters::defaultMorph };
    std::atomic<float> drive{ Parameters::defaultDrive };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformDisplay)
};
