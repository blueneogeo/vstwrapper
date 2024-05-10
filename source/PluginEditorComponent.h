#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
        : editor (std::move (editorIn))
    {
        addAndMakeVisible (editor.get());
        addAndMakeVisible (closeButton);

        childBoundsChanged (editor.get());

        closeButton.onClick = std::forward<Callback> (onClose);
    }

    void setScaleFactor (float scale);

    void resized() override;

    void childBoundsChanged (Component* child) override;

private:
    static constexpr auto buttonHeight = 40;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton closeButton { "Close Plugin" };
};
