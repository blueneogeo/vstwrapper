#pragma once

#include <mach-o/dyld.h>
#include <vector>

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
enum class EditorStyle { thisWindow,
    newWindow };

//==============================================================================

constexpr auto margin = 10;

static void doLayout (juce::Component* main, juce::Component& bottom, int bottomHeight, juce::Rectangle<int> bounds)
{
    juce::Grid grid;
    grid.setGap (juce::Grid::Px { margin });
    grid.templateColumns = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } } };
    grid.templateRows = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } },
        juce::Grid::TrackInfo { juce::Grid::Px { bottomHeight } } };
    grid.items = { juce::GridItem { main }, juce::GridItem { bottom }.withMargin ({ 0, margin, margin, margin }) };
    grid.performLayout (bounds);
}


static std::string getPluginPath() {
    uint32_t size = 1024;
    std::vector<char> buffer(size);
    
    if (_NSGetExecutablePath(buffer.data(), &size) == -1) {
        buffer.resize(size);
        _NSGetExecutablePath(buffer.data(), &size);
    }
    
    return std::string(buffer.data());
}
