#include "ParameterHeuristics.h"
#include "EditorTools.h"
#include <memory>

void analyseParameter (juce::AudioProcessorParameter* param, ParamMetaData* data)
{
    data->name = param->getName (256);

    size_t MAX_OPTIONS = 220;

    auto choices = findChoices (param, MAX_OPTIONS);

    data->isBoolean = choices->size() == 2;

    data->isDiscrete = choices->size() < MAX_OPTIONS;

    if (data->isDiscrete)
    {
        data->choices = choices;
    }

    if (choices->size() >= 1)
    {
        auto first = choices->at (0);
        auto last = choices->at (choices->size() - 1);

        data->startLabel = first->label;
        data->endLabel = last->label;

        auto firstData = analyseParamLabel (first->label);
        auto lastData = analyseParamLabel (last->label);

        data->unit = firstData->unit;
        data->start = firstData->value;
        data->end = lastData->value;
        data->isInt = lastData->isInt;
    }
}

shared_ptr<Choices> findChoices (juce::AudioProcessorParameter* param, size_t steps)
{
    auto choices = make_shared<Choices>();

    float step = 1.0f / static_cast<float> (steps);
    juce::String lastValue = "";

    for (float value = 0.0f; value <= 1.0f; value += step)
    {
        param->setValue (value);
        auto label = param->getCurrentValueAsText();

        // add a new choice if the label is different from the one before
        if (!label.equalsIgnoreCase (lastValue))
        {
            lastValue = label;
            auto choice = make_shared<ParamChoice>();
            choice->value = value;
            choice->label = label;
            choices->push_back (choice);
        }
    }

    return choices;
}

std::shared_ptr<ParamData> analyseParamLabel (String& label)
{
    // std::shared_ptr<Data> extractData(String label) {
    auto data = std::make_shared<ParamData>();

    // Initialize default values
    data->pre = "";
    data->value = 0.0f;
    data->unit = "";
    data->isInt = true;

    int i = 0;

    // Extract prefix (non-numeric part at beginning)
    while (i < label.length() && !(juce::CharacterFunctions::isDigit (label[i]) || label[i] == '-' || label[i] == '.'))
    {
        data->pre += label[i];
        ++i;
    }

    // Skip any spaces after prefix
    while (i < label.length() && juce::CharacterFunctions::isWhitespace (label[i]))
    {
        ++i;
    }

    // Extract numeric value part
    String numberStr;

    while (i < label.length() && (juce::CharacterFunctions::isDigit (label[i]) || label[i] == '.'))
    {
        numberStr += label[i];
        ++i;
    }

    // Check if it's an integer or float based on presence of '.'
    if (numberStr.containsChar ('.'))
    {
        data->value = numberStr.getFloatValue();
        data->isInt = false;
    }
    else
    {
        data->value = static_cast<float> (numberStr.getIntValue());
        data->isInt = true;
    }

    // The rest is considered as unit
    while (i < label.length())
    {
        if (!juce::CharacterFunctions::isWhitespace (label[i]))
        { // Ignore whitespace in unit part
            data->unit += label[i];
        }
        ++i;
    }

    return data;
}

void logToFile (ParamMetaData* data)
{
    logToFile ("Param " + data->name);
    logToFile (" - start: " + static_cast<String>(data->start));
    logToFile (" - end  : " + static_cast<String>(data->end));
    logToFile (" - isInt: " + static_cast<String> (data->isInt ? "true" : "false"));
    logToFile (" - unit: " + data->unit);
    logToFile (" - switch: " + static_cast<String> (data->isBoolean ? "true" : "false"));
    if (data->isDiscrete)
    {
        logToFile (" - discrete choices  : ");
        for (auto choice : *(data->choices))
        {
            logToFile ("    - " + static_cast<String> (choice->value) + ": " + choice->label);
        }
    }
}
