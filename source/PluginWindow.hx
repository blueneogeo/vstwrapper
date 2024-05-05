#pragma once
#include "PluginProcessor.h"

class PluginWindow : public juce::DocumentWindow, public juce::ChangeBroadcaster
{
public:
    PluginWindow(juce::AudioProcessor* thePlugin);
    ~PluginWindow();

    void closeButtonPressed() override;

private:
    juce::AudioProcessor* plugin;
    std::unique_ptr<juce::AudioProcessorEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginWindow)
};
