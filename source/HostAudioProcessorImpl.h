#pragma once

#include "EditorTools.h"
#include "juce_core/juce_core.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include "NRPNReceiver.h"

class HostAudioProcessorImpl : public juce::AudioProcessor,
                               public juce::AudioProcessorListener,
                               private juce::FocusChangeListener,
                               public juce::MidiInputCallback
{
public:
    HostAudioProcessorImpl();

    bool isBusesLayoutSupported (const BusesLayout& layouts) const final;

    void audioProcessorParameterChanged (juce::AudioProcessor* processor,
        int parameterIndex,
        float newValue) override;

    void handleIncomingMidiMessage (juce::MidiInput* source,
        const juce::MidiMessage& message) override;

    void handlePartialSysexMessage (juce::MidiInput* source,
        const juce::uint8* messageData,
        int numBytesSoFar,
        double timestamp) override;

    void handleIncomingNRPN(int parameter, int value);

    void audioProcessorChanged (AudioProcessor* processor, const ChangeDetails& details) override;

    void globalFocusChanged (juce::Component* focusedComponent) override;

    void prepareToPlay (double sr, int bs) final;

    void releaseResources() final;

    void reset() final;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) final;

    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) final;

    bool hasEditor() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    const juce::String getName() const final { return "Electra One"; }

    bool acceptsMidi() const final { return true; }

    bool producesMidi() const final { return true; }

    void setMidiInput(juce::String deviceID);

    void setMidiOutput(juce::String deviceID);
    
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
    // pluginChanged is a callback to inform the HostAudioProcessorEditor
    std::function<void()> pluginChanged;
    juce::Array<juce::AudioProcessorParameter*> pluginParams;

    juce::String midiInputDeviceID = "";
    juce::String midiOutputDeviceID = "";
    std::unique_ptr<juce::MidiInput> midiInput;
    std::unique_ptr<juce::MidiOutput> midiOutput;
    juce::CriticalSection midiInputLock;
    std::unique_ptr<NRPNReceiver> midiReceiver;

private:
    juce::AudioDeviceManager deviceManager;
    bool isUpdatingParam = false;
    juce::CriticalSection innerMutex;
    std::unique_ptr<juce::AudioPluginInstance> inner;
    EditorStyle editorStyle = EditorStyle {};
    bool active = false;
    juce::ScopedMessageBox messageBox;

    static constexpr const char* innerStateTag = "inner_state";
    static constexpr const char* editorStyleTag = "editor_style";

    void changeListenerCallback (juce::ChangeBroadcaster* source);
};
