#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "HostAudioProcessorImpl.h"
#include "PluginLoaderComponent.h"
#include "PluginEditorComponent.h"

class HostAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HostAudioProcessorEditor (HostAudioProcessorImpl& owner);

    void paint (juce::Graphics& g) override;

    void resized() override;

    void childBoundsChanged (Component* child) override;

    void setScaleFactor (float scale) override;

private:
    void pluginChanged();

    void clearPlugin();

    static constexpr auto buttonHeight = 30;

    HostAudioProcessorImpl& hostProcessor;
    PluginLoaderComponent loader;
    std::unique_ptr<Component> editor;
    PluginEditorComponent* currentEditorComponent = nullptr;
    juce::ScopedValueSetter<std::function<void()>> scopedCallback;
    float currentScaleFactor = 1.0f;
};
