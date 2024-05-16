#include <functional>
#include <juce_audio_processors/juce_audio_processors.h>

class ParameterChangeListener : public juce::AudioProcessorParameter::Listener
{
public:
    ParameterChangeListener (std::function<void (int, float)> paramChanged)
        : onParamChanged (paramChanged)
    {
    }

    void parameterValueChanged (int parameterIndex, float newValue)
    {
        if (onParamChanged)
            onParamChanged (parameterIndex, newValue);
    }

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
    {
    }

private:
    std::function<void (int, float)> onParamChanged;
};
