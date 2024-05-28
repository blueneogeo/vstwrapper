#pragma once

#include <functional>
#include <vector>

class ParameterEventBus
{
public:
    using EventCallback = std::function<void (int param, int value)>;

    static void subscribe (EventCallback callback)
    {
        getInstance().listeners.push_back (callback);
    }

    static void publish (int param, int value)
    {
        auto& instance = getInstance();
        for (auto& callback : instance.listeners)
        {
            callback (param, value);
        }
    }

private:
    ParameterEventBus() = default;

    static ParameterEventBus& getInstance()
    {
        static ParameterEventBus instance;
        return instance;
    }

    std::vector<EventCallback> listeners;
};
