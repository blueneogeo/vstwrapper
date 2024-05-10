#include <juce_audio_processors/juce_audio_processors.h>
#include "HostAudioProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HostAudioProcessor();
}
