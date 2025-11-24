#pragma once
#include <JuceHeader.h>

class SubSaverLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SubSaverLookAndFeel()
    {
        // Colori per knobs
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff8a8a8a));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff3a3a3a));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xff8a8a8a));
        
        // Colori per slider lineari
        setColour(juce::Slider::trackColourId, juce::Colour(0xff505050));
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Outer circle (grey, metal look)
        g.setColour(juce::Colour(0xff707070));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Inner shadow
        g.setColour(juce::Colour(0xff3a3a3a));
        g.fillEllipse(centreX - radius * 0.85f, centreY - radius * 0.85f, 
                     radius * 1.7f, radius * 1.7f);

        // Center circle (lighter)
        g.setColour(juce::Colour(0xff909090));
        g.fillEllipse(centreX - radius * 0.7f, centreY - radius * 0.7f, 
                     radius * 1.4f, radius * 1.4f);

        // Indicator dots (small white dots around perimeter)
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        for (int i = 0; i < 11; ++i)
        {
            float dotAngle = rotaryStartAngle + i * (rotaryEndAngle - rotaryStartAngle) / 10.0f;
            float dotX = centreX + (radius + 6) * std::cos(dotAngle - juce::MathConstants<float>::halfPi);
            float dotY = centreY + (radius + 6) * std::sin(dotAngle - juce::MathConstants<float>::halfPi);
            g.fillEllipse(dotX - 2, dotY - 2, 4, 4);
        }

        // Pointer line (white indicator)
        juce::Path pointer;
        pointer.addRectangle(-1.5f, -radius * 0.6f, 3.0f, radius * 0.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(pointer);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            // Vertical slider track (dark background)
            auto trackWidth = 10.0f;
            auto trackX = x + (width - trackWidth) * 0.5f;
            
            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(trackX, y, trackWidth, height, 5.0f);

            // Filled portion (lighter grey)
            float thumbPos = sliderPos;
            g.setColour(juce::Colour(0xff606060));
            g.fillRoundedRectangle(trackX, thumbPos, trackWidth, height - (thumbPos - y), 5.0f);

            // Thumb (small rectangle)
            g.setColour(juce::Colour(0xff909090));
            g.fillRoundedRectangle(trackX - 3, thumbPos - 8, trackWidth + 6, 16, 3.0f);
        }
        else if (style == juce::Slider::LinearHorizontal)
        {
            // Horizontal slider track
            auto trackHeight = 10.0f;
            auto trackY = y + (height - trackHeight) * 0.5f;

            g.setColour(juce::Colour(0xff2a2a2a));
            g.fillRoundedRectangle(x, trackY, width, trackHeight, 5.0f);

            // Filled portion
            g.setColour(juce::Colour(0xff606060));
            g.fillRoundedRectangle(x, trackY, sliderPos - x, trackHeight, 5.0f);

            // Thumb
            g.setColour(juce::Colour(0xff909090));
            g.fillRoundedRectangle(sliderPos - 8, trackY - 3, 16, trackHeight + 6, 3.0f);
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1);
        auto cornerSize = 3.0f;

        // Background (più scuro quando off, più chiaro quando on)
        if (button.getToggleState())
        {
            g.setColour(juce::Colour(0xff5a9a5a)); // Verde quando attivo
        }
        else
        {
            g.setColour(juce::Colour(0xff3a3a3a)); // Grigio scuro quando inattivo
        }
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(juce::Colour(0xff1a1a1a));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Text (più piccolo per il bottone compatto)
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }

};
