#pragma once

#include <mach-o/dyld.h>
#include <vector>

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

const bool LOG = true;
const juce::String LOGFILE = "ElectraLog.txt";

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

inline juce::File getLogFile()
{
    auto locationDir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    return locationDir.getChildFile (LOGFILE);
}

inline void clearLogFile()
{
    auto logFile = getLogFile();
    if (logFile.existsAsFile())
    {
        logFile.deleteFile();
    }
}

inline void logToFile (const juce::String& message)
{
    if (!LOG)
        return;

    auto logFile = getLogFile();
    logFile.appendText (message + "\n");
}
