#pragma once

#include "EditorTools.h"
#include "HostAudioProcessorImpl.h"
#include "Images.h"
#include "LookAndFeel.h"
#include "EventBus.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (HostAudioProcessorImpl* hostAudioProcessor, std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
        : processor (hostAudioProcessor), editor (std::move (editorIn))
    {
        auto colors = juce::LookAndFeel_V4::getDarkColourScheme();
        auto textColor = juce::Colour::fromRGB (240, 240, 240);
        auto outlineColor = juce::Colour::fromRGB (70, 70, 70);
        colors.setUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::defaultText, textColor);
        colors.setUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::outline, outlineColor);
        colors.setUIColour (juce::LookAndFeel_V4::ColourScheme::UIColour::menuText, textColor);
        lookAndFeel = std::make_unique<CustomLookAndFeel> (12.0f, colors);

        auto svg = juce::parseXML (electraOneSVGLogo);
        svgLogo = juce::Drawable::createFromSVG (*svg);
        juce::Rectangle<float> targetBounds (0, 0, 60, 20);
        svgLogo->setTransformToFit (targetBounds, juce::RectanglePlacement::centred);

        auto midiInputs = juce::MidiInput::getAvailableDevices();
        midiInputSelector.setText ("Electra Midi IN");
        for (int i = 0; i < midiInputs.size(); i++)
        {
            auto device = midiInputs[i];
            juce::String deviceName = device.name;
            auto deviceID = device.identifier;
            midiInputSelector.addItem (deviceName, i + 1);
            if (deviceID == processor->midiInputDeviceID)
            {
                midiInputSelector.setSelectedItemIndex (i);
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
            if (deviceID == processor->midiOutputDeviceID)
            {
                midiOutputSelector.setSelectedItemIndex (i);
            }
        }

        midiChannelSelector.setText ("Channel");
        for (int i = 1; i <= 16; i++)
        {
            midiChannelSelector.addItem ("channel " + static_cast<juce::String> (i), i);
            if (i == processor->midiChannelID)
            {
                midiChannelSelector.setSelectedId(i);
            }
        }

        electraSlotSelector.setText ("Slot");
        for (int i = 1; i <= 12; i++)
        {
            electraSlotSelector.addItem ("Preset " + static_cast<juce::String> (i), i);
            if (i == processor->presetSlotID)
            {
                electraSlotSelector.setSelectedId (i);
            }
        }

        paramLabel.setText("", juce::NotificationType::dontSendNotification);
        paramLabel.setFont(juce::Font(12));
        
        midiInputSelector.setLookAndFeel (lookAndFeel.get());
        midiOutputSelector.setLookAndFeel (lookAndFeel.get());
        midiChannelSelector.setLookAndFeel (lookAndFeel.get());
        electraSlotSelector.setLookAndFeel (lookAndFeel.get());
        paramLabel.setLookAndFeel(lookAndFeel.get());
        ejectButton.setLookAndFeel (lookAndFeel.get());

        addAndMakeVisible (editor.get());
        addAndMakeVisible (midiInputSelector);
        addAndMakeVisible (midiOutputSelector);
        addAndMakeVisible (midiChannelSelector);
        addAndMakeVisible (electraSlotSelector);
        addAndMakeVisible (paramLabel);
        addAndMakeVisible (ejectButton);

        childBoundsChanged (editor.get());

        ejectButton.onClick = std::forward<Callback> (onClose);

        midiInputSelector.onChange = [this] {
            logToFile ("selected input " + static_cast<juce::String> (midiInputSelector.getSelectedId()));
            if (midiInputSelector.getSelectedId() == 0)
                return;
            auto inputs = juce::MidiInput::getAvailableDevices();
            auto deviceIndex = midiInputSelector.getSelectedId();
            auto input = inputs[deviceIndex - 1];
            logToFile ("selected input name " + input.name);
            processor->setMidiInput (input.identifier);
        };

        midiOutputSelector.onChange = [this] {
            logToFile ("selected output " + static_cast<juce::String> (midiOutputSelector.getSelectedId()));
            if (midiOutputSelector.getSelectedId() == 0)
                return;
            auto outputs = juce::MidiOutput::getAvailableDevices();
            auto deviceIndex = midiOutputSelector.getSelectedId();
            auto output = outputs[deviceIndex - 1];
            logToFile ("selected output name " + output.name);
            processor->setMidiOutput (output.identifier);
        };

        midiChannelSelector.onChange = [this] {
            processor->midiChannelID = midiChannelSelector.getSelectedId();
            logToFile("set midi channel to " + static_cast<juce::String>(processor->midiChannelID));
        };

        electraSlotSelector.onChange = [this] {
            processor->presetSlotID = electraSlotSelector.getSelectedId();
            logToFile("set preset to " + static_cast<juce::String>(processor->presetSlotID));
        };

        ParameterEventBus::subscribe([this](int param, int value) { onParameterChanged(param, value); });
    }

    void onParameterChanged(int param, int value);

    void setScaleFactor (float scale);

    void resized() override;

    void childBoundsChanged (Component* child) override;

    void paint (juce::Graphics& g) override;

private:
    HostAudioProcessorImpl* processor;

    static constexpr auto toolbarHeight = 20;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    std::unique_ptr<juce::Drawable> svgLogo;
    juce::ComboBox midiInputSelector;
    juce::ComboBox midiOutputSelector;
    juce::ComboBox midiChannelSelector;
    juce::ComboBox electraSlotSelector;
    juce::Label paramLabel;
    juce::TextButton ejectButton { "Eject" };
    std::unique_ptr<CustomLookAndFeel> lookAndFeel;
};
