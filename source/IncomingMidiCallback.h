#pragma once

#include "HostAudioProcessorImpl.h"
#include "ParameterChangeListener.h"
#include "juce_audio_devices/juce_audio_devices.h"

class IncomingMidiMessageCallback : public juce::CallbackMessage
{
public:
    IncomingMidiMessageCallback(HostAudioProcessorImpl* processor, juce::MidiInput* source, const juce::MidiMessage& message)
        : proc(processor), src(source), msg(message) {}

    void messageCallback() override
    {
        proc->handleIncomingMidiMessage(src, msg);
    }

private:
    HostAudioProcessorImpl* proc;
    juce::MidiInput* src;
    const juce::MidiMessage msg;
};
