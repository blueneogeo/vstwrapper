#include "PluginEditorComponent.h"
#include "EditorTools.h"
#include "juce_events/juce_events.h"
#include <memory>
#include <string>

void PluginEditorComponent::setScaleFactor (float scale)
{
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized()
{
    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop (toolbarHeight + 10).reduced (7, 5);
    auto inner = toolbar.removeFromTop (toolbarHeight);

    inner.removeFromLeft (80); // Electra One logo space

    // disable the input selectors for now
    // midiInputSelector.setBounds (inner.removeFromLeft (140));
    // inner.removeFromLeft (7);
    // midiOutputSelector.setBounds (inner.removeFromLeft (140));
    // inner.removeFromLeft (7);

    if (!isRestartRequired && BANK == 0 && SLOT == 0)
    {
        midiChannelSelector.setBounds (inner.removeFromLeft (100));
        inner.removeFromLeft (7);
        electraSlotSelector.setBounds (inner.removeFromLeft (100));
        inner.removeFromLeft (7);
    }

    statusLabel.setBounds (inner.removeFromLeft (120));
    ejectButton.setBounds (inner.removeFromRight (50));

    auto isElectraFound = processor->midiInputDeviceID.isNotEmpty() && processor->midiOutputDeviceID.isNotEmpty();

    juce::String status = "";

    if (isRestartRequired)
    {
        status = "Please restart this plugin";
    }
    else if (isElectraFound || !isMidiChecked)
    {
        if (BANK > 0 && SLOT > 0)
        {
            status = "bank " + std::to_string (BANK) + " slot " + std::to_string (SLOT);
        }
        else
        {
            status = "";
        }
    }
    else
    {
        status = "Electra not found";
    }

    statusLabel.setText (status, juce::NotificationType::dontSendNotification);

    if (editor)
    {
        editor->setBounds (area);
    }
}

// periodically check for the Electra controller input and output
void PluginEditorComponent::timerCallback()
{
    const auto MIDI_IN = "Electra Controller Electra Port 2";
    const auto MIDI_OUT = "Electra Controller Electra Port 2";

    auto midiInputs = juce::MidiInput::getAvailableDevices();
    juce::String inDevice = "";
    for (int i = 0; i < midiInputs.size(); i++)
    {
        auto device = midiInputs[i];
        juce::String deviceName = device.name;
        if (deviceName.equalsIgnoreCase (MIDI_IN))
        {
            inDevice = device.identifier;
        }
    }

    if (inDevice.isEmpty())
    {
        processor->clearMidiInput();
    }
    else
    {
        processor->setMidiInput (inDevice);
    }

    auto midiOutputs = juce::MidiOutput::getAvailableDevices();
    juce::String outDevice = "";
    for (int i = 0; i < midiOutputs.size(); i++)
    {
        auto device = midiOutputs[i];
        juce::String deviceName = device.name;
        if (deviceName.equalsIgnoreCase (MIDI_OUT))
        {
            outDevice = device.identifier;
        }
    }

    if (outDevice.isEmpty())
    {
        processor->clearMidiOutput();
    }
    else
    {
        processor->setMidiOutput (outDevice);
    }

    resized();

    isMidiChecked = true;
}

void PluginEditorComponent::onParameterChanged (int param, int value)
{
    if (isRestartRequired)
        return;

    std::string text = std::to_string (param + 1) + " - " + std::to_string (value);
    statusLabel.setText (text, juce::NotificationType::sendNotification);
}

void PluginEditorComponent::paint (juce::Graphics& g)
{
    if (svgLogo)
    {
        svgLogo->drawAt (g, 5, 5, 1);
    }
}

void PluginEditorComponent::childBoundsChanged (Component* child)
{
    if (child != editor.get())
        return;

    const auto size = editor != nullptr ? editor->getLocalBounds()
                                        : juce::Rectangle<int>();

    setSize (size.getWidth(), margin + toolbarHeight + size.getHeight());
}
