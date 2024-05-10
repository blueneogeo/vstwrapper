#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ScaledDocumentWindow final : public juce::DocumentWindow
{
public:
    ScaledDocumentWindow (juce::Colour bg, float scale)
        : DocumentWindow ("Editor", bg, 0), desktopScale (scale) {}

    float getDesktopScaleFactor() const override { return juce::Desktop::getInstance().getGlobalScaleFactor() * desktopScale; }

private:
    float desktopScale = 1.0f;
};
