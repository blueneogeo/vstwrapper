#include "PluginEditorComponent.h"
#include "EditorTools.h"
#include "HostAudioProcessorImpl.h"
#include "NRPNReceiver.h"
#include <memory>

void PluginEditorComponent::setScaleFactor (float scale)
{
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized()
{
    const int width = 150;

    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop (toolbarHeight + 10).reduced (7, 5);
    auto inner = toolbar.removeFromTop (toolbarHeight);

    midiInputSelector.setBounds (inner.removeFromLeft (width));
    inner.removeFromLeft (7);
    midiOutputSelector.setBounds (inner.removeFromLeft (width));

    inner.removeFromLeft (7);
    testButton.setBounds (inner.removeFromLeft (50));

    closeButton.setBounds (inner.removeFromRight (50));

    if (editor)
    {
        editor->setBounds (area);
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

void PluginEditorComponent::midiInputChanged()
{
    auto selectedIndex = midiInputSelector.getSelectedItemIndex();

    if (selectedIndex >= 0)
    {
        DBG ("MIDI Input selected: " + juce::MidiInput::getAvailableDevices()[selectedIndex].name);
        if (editor)
        {
            auto input = juce::MidiInput::getAvailableDevices()[selectedIndex];
            logToFile ("midi in device found: " + input.name + " : " + input.identifier);
            auto midiDevice = juce::MidiInput::openDevice (input.identifier, processor);
            if (midiDevice == nullptr)
            {
                logToFile ("midi device not found");
            }
            else
            {
                if(processor->midiInput != nullptr) {
                    audioDeviceManager.removeMidiInputDeviceCallback(processor->midiInput->getIdentifier(), processor);
                }
                logToFile ("midi device found and created: " + midiDevice->getName());
                processor->midiInput = std::move (midiDevice);
                processor->midiReceiver = std::make_unique<NRPNReceiver> (1, [this] (int param, int value) {
                    this->processor->handleIncomingNRPN (param, value);
                });
                audioDeviceManager.setMidiInputDeviceEnabled(input.identifier, true);
                audioDeviceManager.addMidiInputDeviceCallback(input.identifier, processor);
            }
        }
    }
}

void PluginEditorComponent::midiOutputChanged()
{
    auto selectedIndex = midiOutputSelector.getSelectedItemIndex();

    if (selectedIndex >= 0 && editor)
    {
        juce::ScopedLock lock (processor->midiInputLock);
        if (processor->midiInput)
        {
            processor->midiInput.reset();
        }

        auto output = juce::MidiOutput::getAvailableDevices()[selectedIndex];
        logToFile ("midi out device found: " + output.name + " : " + output.identifier);
        auto midiDevice = juce::MidiOutput::openDevice (output.identifier);
        if (midiDevice == nullptr)
        {
            logToFile ("midi output device not found");
        }
        else
        {
            logToFile ("midi output device found and created");
            logToFile (midiDevice->getName());
            processor->midiOutput = std::move (midiDevice);
        }
    }
}
