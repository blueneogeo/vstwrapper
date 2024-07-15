#include "ParamMetaData.h"
#include "EditorTools.h"
#include <memory>

shared_ptr<ParamMetaData> analyseParameter (juce::AudioProcessorParameter* param)
{
    size_t MAX_OPTIONS = 220;

    auto data = make_shared<ParamMetaData>();

    data->name = param->getName (256);

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
        data->startValue = firstData->value;
        data->endValue = lastData->value;
        data->isInt = lastData->isInt;
    }

    return data;
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
    auto data = std::make_shared<ParamData>();

    // Initialize default values
    data->pre = "";
    data->value = 0.0f;
    data->unit = "";
    data->isInt = false;

    int i = 0;

    // Extract prefix (non-numeric part at beginning)
    while (i < label.length() && !(juce::CharacterFunctions::isDigit (label[i]) || label[i] == '-' || label[i] == '.'))
    {
        data->pre += label[i];
        ++i;
    }

    String numberStr; // Declare numberStr here

    // Skip any spaces after prefix
    while (i < label.length() && juce::CharacterFunctions::isWhitespace (label[i]))
    {
        ++i;
    }

    // Handle negative numbers directly before extracting numeric value part
    if (i < label.length() && (label[i] == '-'))
    {
        numberStr += label[i];
        ++i;
    }

    // Extract numeric value part
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

juce::XmlElement* toParamDataXML (shared_ptr<ParamMetaData> paramData)
{
    auto paramEl = new juce::XmlElement ("param");
    paramEl->setAttribute ("name", paramData->name);
    paramEl->setAttribute ("label", paramData->label);
    paramEl->setAttribute ("default", paramData->defaultValue);
    paramEl->setAttribute ("discrete", paramData->isDiscrete);
    if (paramData->isDiscrete)
    {
        paramEl->setAttribute ("isbutton", paramData->isBoolean);
        auto choices = new juce::XmlElement ("choices");
        for (auto choice : *(paramData->choices))
        {
            auto choiceEl = new juce::XmlElement ("choice");
            choiceEl->setAttribute ("label", choice->label);
            choiceEl->setAttribute ("value", choice->value);
            choices->addChildElement (choiceEl);
        }
        paramEl->addChildElement (choices);
    }
    else
    {
        paramEl->setAttribute ("startValue", paramData->startValue);
        paramEl->setAttribute ("startLabel", paramData->startLabel);
        paramEl->setAttribute ("endValue", paramData->endValue);
        paramEl->setAttribute ("endLabel", paramData->endLabel);
        paramEl->setAttribute ("unit", paramData->unit);
        paramEl->setAttribute ("isInt", paramData->isInt);
    }

    return paramEl;
}

shared_ptr<ParamMetaData> toParamMetaData (juce::XmlElement* parameterEl)
{
    // parse parameter metadata
    const auto paramData = make_shared<ParamMetaData>();

    paramData->name = parameterEl->getStringAttribute ("name");
    paramData->label = parameterEl->getStringAttribute ("label");
    paramData->defaultValue = static_cast<float> (parameterEl->getDoubleAttribute ("default", 0.0));
    paramData->isDiscrete = parameterEl->getBoolAttribute ("discrete", false);

    if (paramData->isDiscrete)
    {
        paramData->isBoolean = parameterEl->getBoolAttribute ("isbutton", false);
        auto choiceEls = parameterEl->getChildByName ("choices");
        if (choiceEls)
        {
            paramData->choices = make_shared<Choices>();
            for (int c = 0; c < choiceEls->getNumChildElements(); c++)
            {
                auto choiceEl = choiceEls->getChildElement (c);
                auto choice = make_shared<ParamChoice>();
                choice->label = choiceEl->getStringAttribute ("label");
                choice->value = static_cast<float> (choiceEl->getDoubleAttribute ("value", 0.0));
                paramData->choices->push_back (choice);
            }
        }
    }
    else
    {
        paramData->startValue = static_cast<float> (parameterEl->getDoubleAttribute ("startValue", 0.0));
        paramData->endValue = static_cast<float> (parameterEl->getDoubleAttribute ("endValue", 0.0));
        paramData->startLabel = parameterEl->getStringAttribute ("startLabel");
        paramData->endLabel = parameterEl->getStringAttribute ("endLabel");
        paramData->unit = parameterEl->getStringAttribute ("unit");
        paramData->isInt = parameterEl->getBoolAttribute ("isInt", false);
    }

    // logToFile (paramData.get());

    return paramData;
}

void logToFile (ParamMetaData* data)
{
    logToFile ("Param " + data->name);
    if (data->isDiscrete)
    {
        logToFile (" - is button: " + static_cast<String> (data->isBoolean ? "true" : "false"));
        logToFile (" - discrete steps: " + static_cast<String> (data->choices->size()));
        logToFile (" - discrete choices  : ");
        for (auto choice : *(data->choices))
        {
            logToFile ("    - " + static_cast<String> (choice->value) + ": " + choice->label);
        }
    }
    else
    {
        logToFile (" - continuous");
        logToFile (" - isInt: " + static_cast<String> (data->isInt ? "true" : "false"));
        logToFile (" - start: " + static_cast<String> (data->startValue));
        logToFile (" - end  : " + static_cast<String> (data->endValue));
        logToFile (" - unit: " + data->unit);
    }
    logToFile ("");
}
