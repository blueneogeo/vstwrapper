#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "EditorTools.h"

class HostAudioProcessorImpl : public juce::AudioProcessor,
                               private juce::ChangeListener
{
public:
    HostAudioProcessorImpl();

    bool isBusesLayoutSupported (const BusesLayout& layouts) const final;

    void prepareToPlay (double sr, int bs) final;

    void releaseResources() final;

    void reset() final;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) final;

    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) final;

    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    const juce::String getName() const final { return "HostPluginDemo"; }

    bool acceptsMidi() const final { return true; }

    bool producesMidi() const final { return true; }

    double getTailLengthSeconds() const final { return 0.0; }

    int getNumPrograms() final { return 0; }

    int getCurrentProgram() final { return 0; }

    void setCurrentProgram (int) final {}

    const juce::String getProgramName (int) final { return "None"; }

    void changeProgramName (int, const juce::String&) final {}

    void getStateInformation (juce::MemoryBlock& destData) final;

    void setStateInformation (const void* data, int sizeInBytes) final;

    void setNewPlugin (const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb = {});

    void clearPlugin();

    bool isPluginLoaded() const;

    std::unique_ptr<juce::AudioProcessorEditor> createInnerEditor() const;

    EditorStyle getEditorStyle() const noexcept { return editorStyle; }

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList pluginList;
    std::function<void()> pluginChanged;

private:
    juce::CriticalSection innerMutex;
    std::unique_ptr<juce::AudioPluginInstance> inner;
    EditorStyle editorStyle = EditorStyle {};
    bool active = false;
    juce::ScopedMessageBox messageBox;

    static constexpr const char* innerStateTag = "inner_state";
    static constexpr const char* editorStyleTag = "editor_style";

    void changeListenerCallback (juce::ChangeBroadcaster* source) final;
};