#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include "LookAndFeel.h"

class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
        : editor (std::move (editorIn))
    {
        lookAndFeel = std::make_unique<CustomLookAndFeel>(12.0f);
        audioDeviceManager.initialise (2, 2, nullptr, true);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        midiInputSelector.setText ("Electra Midi IN");
        for (int i = 0; i < midiInputs.size(); i++)
        {
            auto device = midiInputs[i];
            juce::String deviceName = device.name;
            auto deviceID = device.identifier;
            midiInputSelector.addItem (deviceName, i + 1);
        }

        auto midiOutputs = juce::MidiOutput::getAvailableDevices();
        midiOutputSelector.setText ("Electra Midi OUT");
        for (int i = 0; i < midiOutputs.size(); i++)
        {
            auto device = midiOutputs[i];
            juce::String deviceName = device.name;
            auto deviceID = device.identifier;
            midiOutputSelector.addItem (deviceName, i + 1);
        }

        midiInputSelector.setLookAndFeel(lookAndFeel.get());
        midiOutputSelector.setLookAndFeel(lookAndFeel.get());
        closeButton.setLookAndFeel(lookAndFeel.get());
        
        addAndMakeVisible (editor.get());
        addAndMakeVisible (midiInputSelector);
        addAndMakeVisible (midiOutputSelector);
        addAndMakeVisible (closeButton);

        childBoundsChanged (editor.get());

        closeButton.onClick = std::forward<Callback> (onClose);

        midiInputSelector.onChange = [this] { midiInputChanged(); };
        midiOutputSelector.onChange = [this] { midiOutputChanged(); };
    }

    void setScaleFactor (float scale);

    void resized() override;

    void childBoundsChanged (Component* child) override;

    void midiInputChanged();

    void midiOutputChanged();

private:
    static constexpr auto toolbarHeight = 20;

    juce::AudioDeviceManager audioDeviceManager;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::ComboBox midiInputSelector;
    juce::ComboBox midiOutputSelector;
    juce::TextButton closeButton { "Eject" };
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
};
