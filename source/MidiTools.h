#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>

inline void sendNRPN(juce::MidiOutput* midiOutput, int channel, int parameter, int value)
{
    int parameterMSB = (parameter >> 7) & 0x7F;
    int parameterLSB = parameter & 0x7F;

    int valueMSB = (value >> 7) & 0x7F;
    int valueLSB = value & 0x7F;
    
    // Ensure the channel is within the valid range (1-16)
    jassert(channel >= 1 && channel <= 16);

    // Create the NRPN message sequence
    juce::MidiMessageSequence sequence;

    // Send CC 99 (NRPN MSB) with parameterMSB
    sequence.addEvent(juce::MidiMessage::controllerEvent(channel, 99, parameterMSB));
    
    // Send CC 98 (NRPN LSB) with parameterLSB
    sequence.addEvent(juce::MidiMessage::controllerEvent(channel, 98, parameterLSB));
    
    // Send CC 6 (Data Entry MSB) with valueMSB
    sequence.addEvent(juce::MidiMessage::controllerEvent(channel, 6, valueMSB));

    // Optionally send CC 38 (Data Entry LSB) with valueLSB if needed
    if (valueLSB >= 0)
        sequence.addEvent(juce::MidiMessage::controllerEvent(channel, 38, valueLSB));

    // Send all messages in the sequence via MIDI output
    for (int i = 0; i < sequence.getNumEvents(); ++i)
        midiOutput->sendMessageNow(sequence.getEventPointer(i)->message);
}
