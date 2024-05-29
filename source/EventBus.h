#pragma once

#include <functional>
#include <memory>
#include <mutex>

class ParameterEventBus
{
public:
    using EventCallback = std::function<void (int param, int value)>;
    using CallbackPtr = std::shared_ptr<EventCallback>;

    static CallbackPtr subscribe (EventCallback callback)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);
        auto cbPtr = std::make_shared<EventCallback> (std::move (callback));
        instance.listener = cbPtr;
        return cbPtr;
    }

    static void unsubscribe()
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);
        instance.listener.reset();
    }

    static void publish (int param, int value)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);

        if (instance.listener && *instance.listener)
        {
            (*instance.listener) (param, value);
        }
    }

private:
    ParameterEventBus() = default;

    static ParameterEventBus& getInstance()
    {
        static ParameterEventBus instance;
        return instance;
    }

private:
    CallbackPtr listener;
    mutable std::mutex mutex;
};
