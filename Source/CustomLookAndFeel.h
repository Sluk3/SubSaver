#pragma once
#include <JuceHeader.h>

class SubSaverLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SubSaverLookAndFeel()
    {
        // Colori per knobs (scuri con accent rosso)
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff2a2e35));
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1d22));
        setColour(juce::Slider::thumbColourId, juce::Colour(0xffff6b6b));

        // Colori per slider lineari
        setColour(juce::Slider::trackColourId, juce::Colour(0xff2a2e35));
        setColour(juce::Slider::backgroundColourId, juce::Colour(0xff1a1d22));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto radius = juce::jmin(width / 2, height / 2) - 8.0f;
        auto centreX = x + width * 0.5f;
        auto centreY = y + height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Glow molto sottile (solo 2 layer leggeri)
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.05f));
        g.fillEllipse(centreX - radius * 1.15f, centreY - radius * 1.15f,
            radius * 2.3f, radius * 2.3f);
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.03f));
        g.fillEllipse(centreX - radius * 1.25f, centreY - radius * 1.25f,
            radius * 2.5f, radius * 2.5f);

        // Shadow/depth layer
        g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
        g.fillEllipse(centreX - radius + 2, centreY - radius + 2,
            radius * 2.0f, radius * 2.0f);

        // Main body (grigio scuro metallico con gradiente)
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff3a3e45), centreX - radius * 0.5f, centreY - radius * 0.5f,
            juce::Colour(0xff22262d), centreX + radius * 0.5f, centreY + radius * 0.5f,
            true));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        // Inner shadow ring
        g.setColour(juce::Colour(0xff000000).withAlpha(0.3f));
        g.drawEllipse(centreX - radius + 1, centreY - radius + 1,
            (radius - 1) * 2.0f, (radius - 1) * 2.0f, 1.0f);

        // Track background (anello scuro dove va il progresso)
        float trackWidth = 6.0f;
        float trackRadius = radius * 0.92f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, trackRadius, trackRadius,
            0.0f, rotaryStartAngle, rotaryEndAngle, true);

        g.setColour(juce::Colour(0xff1a1d22));
        juce::PathStrokeType backgroundStroke(trackWidth, juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded);
        g.strokePath(backgroundArc, backgroundStroke);

        // Progress arc con glow minimo
        juce::Path progressArc;
        progressArc.addCentredArc(centreX, centreY, trackRadius, trackRadius,
            0.0f, rotaryStartAngle, angle, true);

        // Glow minimo (solo 1 layer molto sottile)
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.15f));
        juce::PathStrokeType glowStroke(trackWidth + 4.0f, juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded);
        g.strokePath(progressArc, glowStroke);

        // Progress arc principale
        g.setColour(juce::Colour(0xffff6b6b));
        juce::PathStrokeType progressStroke(trackWidth, juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded);
        g.strokePath(progressArc, progressStroke);

        // Inner circle (più scuro)
        float innerRadius = radius * 0.7f;
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xff2a2e35), centreX, centreY - innerRadius,
            juce::Colour(0xff1a1d22), centreX, centreY + innerRadius,
            false));
        g.fillEllipse(centreX - innerRadius, centreY - innerRadius,
            innerRadius * 2.0f, innerRadius * 2.0f);

        // Pointer indicator (linea sottile rosso luminoso)
        juce::Path pointer;
        pointer.addRoundedRectangle(-1.5f, -innerRadius * 0.85f, 3.0f, innerRadius * 0.55f, 1.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        // Glow minimo per l'indicatore
        g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.3f));
        juce::Path pointerGlow = pointer;
        pointerGlow.applyTransform(juce::AffineTransform::scale(1.2f, 1.2f, centreX, centreY));
        g.fillPath(pointerGlow);

        // Indicatore solido
        g.setColour(juce::Colour(0xffff8989));
        g.fillPath(pointer);

        // Center cap (piccolo cerchio centrale)
        float capRadius = 4.0f;
        g.setColour(juce::Colour(0xff1a1d22));
        g.fillEllipse(centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        g.setColour(juce::Colour(0xff2a2e35));
        g.fillEllipse(centreX - capRadius * 0.7f, centreY - capRadius * 0.7f,
            capRadius * 1.4f, capRadius * 1.4f);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            auto trackWidth = 12.0f;
            auto trackX = x + (width - trackWidth) * 0.5f;

            // Glow minimo
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.04f));
            g.fillRoundedRectangle(trackX - 3, y, trackWidth + 6, height, (trackWidth + 6) * 0.5f);

            // Shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.4f));
            g.fillRoundedRectangle(trackX + 1, y + 1, trackWidth, height, trackWidth * 0.5f);

            // Background track (scuro)
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff1a1d22), trackX, y,
                juce::Colour(0xff2a2e35), trackX + trackWidth, y,
                false));
            g.fillRoundedRectangle(trackX, y, trackWidth, height, trackWidth * 0.5f);

            // Inner shadow
            g.setColour(juce::Colour(0xff0f1215).withAlpha(0.6f));
            g.fillRoundedRectangle(trackX + 1, y + 1, trackWidth - 2, height - 2, (trackWidth - 2) * 0.5f);

            // Filled portion con glow minimo
            float thumbPos = sliderPos;
            float fillHeight = height - (thumbPos - y);

            // Glow interno minimo
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.2f));
            g.fillRoundedRectangle(trackX - 1, thumbPos, trackWidth + 2, fillHeight, (trackWidth + 2) * 0.5f);

            // Filled portion main
            g.setColour(juce::Colour(0xffff6b6b));
            g.fillRoundedRectangle(trackX + 2, thumbPos, trackWidth - 4, fillHeight - 2, (trackWidth - 4) * 0.5f);

            // Thumb con glow minimo
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.12f));
            g.fillRoundedRectangle(trackX - 5, thumbPos - 12, trackWidth + 10, 24, 6.0f);

            // Thumb shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
            g.fillRoundedRectangle(trackX - 3 + 1, thumbPos - 10 + 1, trackWidth + 6, 20, 5.0f);

            // Thumb body
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff3a3e45), trackX, thumbPos - 10,
                juce::Colour(0xff2a2e35), trackX, thumbPos + 10,
                false));
            g.fillRoundedRectangle(trackX - 3, thumbPos - 10, trackWidth + 6, 20, 5.0f);

            // Thumb highlight
            g.setColour(juce::Colour(0xff4a4e55).withAlpha(0.4f));
            g.fillRoundedRectangle(trackX - 2, thumbPos - 9, trackWidth + 4, 9, 4.0f);
        }
        else if (style == juce::Slider::LinearHorizontal)
        {
            auto trackHeight = 12.0f;
            auto trackY = y + (height - trackHeight) * 0.5f;

            // Glow minimo
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.04f));
            g.fillRoundedRectangle(x, trackY - 3, width, trackHeight + 6, (trackHeight + 6) * 0.5f);

            // Shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.4f));
            g.fillRoundedRectangle(x + 1, trackY + 1, width, trackHeight, trackHeight * 0.5f);

            // Background track
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff1a1d22), x, trackY,
                juce::Colour(0xff2a2e35), x, trackY + trackHeight,
                false));
            g.fillRoundedRectangle(x, trackY, width, trackHeight, trackHeight * 0.5f);

            g.setColour(juce::Colour(0xff0f1215).withAlpha(0.6f));
            g.fillRoundedRectangle(x + 1, trackY + 1, width - 2, trackHeight - 2, (trackHeight - 2) * 0.5f);

            // Filled portion
            float fillWidth = sliderPos - x;

            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.2f));
            g.fillRoundedRectangle(x, trackY - 1, fillWidth, trackHeight + 2, (trackHeight + 2) * 0.5f);

            g.setColour(juce::Colour(0xffff6b6b));
            g.fillRoundedRectangle(x + 2, trackY + 2, fillWidth - 2, trackHeight - 4, (trackHeight - 4) * 0.5f);

            // Thumb glow minimo
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.12f));
            g.fillRoundedRectangle(sliderPos - 12, trackY - 5, 24, trackHeight + 10, 6.0f);

            // Thumb shadow
            g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
            g.fillRoundedRectangle(sliderPos - 10 + 1, trackY - 3 + 1, 20, trackHeight + 6, 5.0f);

            // Thumb body
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff3a3e45), sliderPos - 10, trackY,
                juce::Colour(0xff2a2e35), sliderPos + 10, trackY,
                false));
            g.fillRoundedRectangle(sliderPos - 10, trackY - 3, 20, trackHeight + 6, 5.0f);

            g.setColour(juce::Colour(0xff4a4e55).withAlpha(0.4f));
            g.fillRoundedRectangle(sliderPos - 9, trackY - 2, 9, trackHeight + 4, 4.0f);
        }
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(2);
        auto cornerSize = 5.0f;

        // Glow minimo quando attivo
        if (button.getToggleState())
        {
            g.setColour(juce::Colour(0xffff6b6b).withAlpha(0.15f));
            g.fillRoundedRectangle(bounds.expanded(3), cornerSize + 1.5f);
        }

        // Shadow
        g.setColour(juce::Colour(0xff000000).withAlpha(0.4f));
        g.fillRoundedRectangle(bounds.translated(1, 1), cornerSize);

        // Background
        if (button.getToggleState())
        {
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xffff6b6b), bounds.getCentreX(), bounds.getY(),
                juce::Colour(0xffcc5555), bounds.getCentreX(), bounds.getBottom(),
                false));
        }
        else
        {
            g.setGradientFill(juce::ColourGradient(
                juce::Colour(0xff2a2e35), bounds.getCentreX(), bounds.getY(),
                juce::Colour(0xff1a1d22), bounds.getCentreX(), bounds.getBottom(),
                false));
        }
        g.fillRoundedRectangle(bounds, cornerSize);

        // Border
        g.setColour(juce::Colour(0xff0f1215));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        // Text
        g.setColour(juce::Colours::white.withAlpha(button.getToggleState() ? 1.0f : 0.7f));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
    }
};
