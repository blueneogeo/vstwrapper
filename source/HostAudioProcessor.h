#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "HostAudioProcessorImpl.h"
#include "HostAudioProcessorEditor.h"

class HostAudioProcessor final : public HostAudioProcessorImpl
{
public:
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return new HostAudioProcessorEditor (*this); }
};
