#pragma once

#include "EditorTools.h"
#include "HostAudioProcessorImpl.h"
#include "LookAndFeel.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (HostAudioProcessorImpl* hostAudioProcessor, std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
        : processor(hostAudioProcessor), editor (std::move (editorIn))
    {
        lookAndFeel = std::make_unique<CustomLookAndFeel> (12.0f);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        midiInputSelector.setText ("Electra Midi IN");
        for (int i = 0; i < midiInputs.size(); i++)
        {
            auto device = midiInputs[i];
            juce::String deviceName = device.name;
            auto deviceID = device.identifier;
            midiInputSelector.addItem (deviceName, i + 1);
            if(deviceID == processor->midiInputDeviceID) {
                midiInputSelector.setSelectedItemIndex(i);
            }
        }

        auto midiOutputs = juce::MidiOutput::getAvailableDevices();
        midiOutputSelector.setText ("Electra Midi OUT");
        for (int i = 0; i < midiOutputs.size(); i++)
        {
            auto device = midiOutputs[i];
            juce::String deviceName = device.name;
            auto deviceID = device.identifier;
            midiOutputSelector.addItem (deviceName, i + 1);
            if(deviceID == processor->midiOutputDeviceID) {
                midiOutputSelector.setSelectedItemIndex(i);
            }
        }

        midiChannelSelector.setText("Channel");
        for (int i = 1; i <= 16; i++)
        {
            midiChannelSelector.addItem("channel " + static_cast<juce::String>(i), i);
            if(i == processor->midiChannelID) {
                midiChannelSelector.setSelectedItemIndex(i-1);
            }
        }

        electraSlotSelector.setText("Slot");
        for (int i = 1; i <= 12; i++)
        {
            electraSlotSelector.addItem("Preset " + static_cast<juce::String>(i), i);
            if(i == processor->presetSlotID) {
                electraSlotSelector.setSelectedItemIndex(i-1);
            }
        }

        midiInputSelector.setLookAndFeel (lookAndFeel.get());
        midiOutputSelector.setLookAndFeel (lookAndFeel.get());
        midiChannelSelector.setLookAndFeel (lookAndFeel.get());
        electraSlotSelector.setLookAndFeel (lookAndFeel.get());
        ejectButton.setLookAndFeel (lookAndFeel.get());

        addAndMakeVisible (editor.get());
        addAndMakeVisible (midiInputSelector);
        addAndMakeVisible (midiOutputSelector);
        addAndMakeVisible (midiChannelSelector);
        addAndMakeVisible (electraSlotSelector);
        addAndMakeVisible (ejectButton);

        childBoundsChanged (editor.get());

        ejectButton.onClick = std::forward<Callback> (onClose);

        midiInputSelector.onChange = [this] { 
            logToFile("selected input " + static_cast<juce::String>(midiInputSelector.getSelectedId()));
            if(midiInputSelector.getSelectedId() == 0) return; 
            auto inputs = juce::MidiInput::getAvailableDevices();
            auto deviceIndex = midiInputSelector.getSelectedId();
            auto input = inputs[deviceIndex-1];
            logToFile("selected input name " + input.name);
            processor->setMidiInput(input.identifier);
        };

        midiOutputSelector.onChange = [this] {
            logToFile("selected output " + static_cast<juce::String>(midiOutputSelector.getSelectedId()));
            if(midiOutputSelector.getSelectedId() == 0) return; 
            auto outputs = juce::MidiOutput::getAvailableDevices();
            auto deviceIndex = midiOutputSelector.getSelectedId();
            auto output = outputs[deviceIndex-1];
            logToFile("selected output name " + output.name);
            processor->setMidiOutput(output.identifier);
        };
    }

    void setScaleFactor (float scale);

    void resized() override;

    void childBoundsChanged (Component* child) override;

private:

    HostAudioProcessorImpl* processor;
    
    static constexpr auto toolbarHeight = 20;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::ComboBox midiInputSelector;
    juce::ComboBox midiOutputSelector;
    juce::ComboBox midiChannelSelector;
    juce::ComboBox electraSlotSelector;
    juce::TextButton ejectButton { "Eject" };
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
};
