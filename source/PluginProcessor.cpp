#pragma once

#include "PluginProcessor.h"
#include "WrapperPlugin.h"
#include <juce_audio_processors/juce_audio_processors.h>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HostAudioProcessor();
}
