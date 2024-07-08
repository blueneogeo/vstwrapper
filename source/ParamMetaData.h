#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "juce_core/juce_core.h"
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
    String label = "";
    bool isDiscrete = false;
    bool isBoolean = false;
    bool isInt = false;
    String startLabel = "";
    String endLabel = "";
    float startValue = 0;
    float endValue = 0;
    float defaultValue = 0;
    String unit = "";
    shared_ptr<Choices> choices;
};

shared_ptr<ParamMetaData> analyseParameter (juce::AudioProcessorParameter* param);
std::shared_ptr<ParamData> analyseParamLabel(String& label);
shared_ptr<Choices> findChoices (juce::AudioProcessorParameter* param, size_t steps);

shared_ptr<ParamMetaData> toParamMetaData(juce::XmlElement* paramEl);
juce::XmlElement* toParamDataXML (shared_ptr<ParamMetaData> paramData);

void logToFile(ParamMetaData* data);
