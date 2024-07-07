#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "std_include.h"

struct ParamData {
    String pre;
    float value;
    String unit;
    bool isInt;
};

struct ParamChoice
{
    float value;
    String label;
};

using Choices = vector<shared_ptr<ParamChoice>>;

struct ParamMetaData
{
    String name = "";
    bool isDiscrete = false;
    bool isBoolean = false;
    bool isInt = false;
    String startLabel = "";
    String endLabel = "";
    float start = 0;
    float end = 0;
    String unit = "";
    shared_ptr<Choices> choices;
};

void analyseParameter(juce::AudioProcessorParameter* param, ParamMetaData* data);

void logToFile(ParamMetaData* data);

shared_ptr<Choices> findChoices (juce::AudioProcessorParameter* param, size_t steps);

std::shared_ptr<ParamData> analyseParamLabel(String& label);
