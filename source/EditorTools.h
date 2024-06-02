#pragma once

#include <mach-o/dyld.h>
#include <vector>

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

const bool LOG = TRUE;

//==============================================================================
enum class EditorStyle { thisWindow,
    newWindow };

//==============================================================================

constexpr auto margin = 10;

inline std::string getPluginPath()
{
    uint32_t size = 1024;
    std::vector<char> buffer (size);

    if (_NSGetExecutablePath (buffer.data(), &size) == -1)
    {
        buffer.resize (size);
        _NSGetExecutablePath (buffer.data(), &size);
    }

    return std::string (buffer.data());
}

inline void logToFile (const juce::String& message)
{
    if (!LOG)
        return;

    juce::File logFile = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                             .getChildFile ("ElectraLog.txt");

    logFile.appendText (message + "\n");
}
