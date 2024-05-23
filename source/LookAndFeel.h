#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel (float fontSize, ColourScheme scheme) : comboFontSize (fontSize), buttonFontSize (fontSize)
    {
        setColourScheme (scheme);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (comboFontSize);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (buttonFontSize);
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto cornerSize = box.findParentComponentOfClass<juce::ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        juce::Rectangle<int> boxBounds (0, 0, width, height);

        g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (boxBounds.toFloat(), cornerSize);

        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (boxBounds.toFloat().reduced (0.5f, 0.5f), cornerSize, 1.0f);

        juce::Rectangle<int> arrowZone (width - 25, 0, 20, height);
        juce::Path triangle;

        juce::Rectangle<int> paddedZone = arrowZone.withTrimmedTop (9).withTrimmedBottom (7).withTrimmedLeft (6).withTrimmedRight (6);

        auto color = juce::Colour::fromRGBA (70, 70, 70, (box.isEnabled() ? 255 : 50));

        triangle.addTriangle (
            static_cast<float> (paddedZone.getCentreX()),
            static_cast<float> (paddedZone.getBottom()),
            static_cast<float> (paddedZone.getX()),
            static_cast<float> (paddedZone.getY()),
            static_cast<float> (paddedZone.getRight()),
            static_cast<float> (paddedZone.getY()));
        g.setColour (color);
        g.fillPath (triangle);
        g.setColour (color);
        g.strokePath (triangle, juce::PathStrokeType (2.0f));
    }

private:
    float comboFontSize;
    float buttonFontSize;
};
