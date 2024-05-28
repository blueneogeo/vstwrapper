#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class ParameterChange {
public:
    template<typename Callable>
    ParameterChange(Callable&& onParameterChange) : onChange(std::forward<Callable>(onParameterChange)) {}

    void execute(int parameter, int value) const {
        if (onChange) {
            onChange(parameter, value);
        }
    }

private:
    std::function<void(int, int)> onChange;
};
