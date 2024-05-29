#pragma once

#include <functional>
#include <memory>
#include <mutex>

class ParameterEventBus
{
public:
    using EventCallback = std::function<void(int param, int value)>;
    using CallbackPtr = std::shared_ptr<EventCallback>;

    // Subscribe to the event bus
    static CallbackPtr subscribe(EventCallback callback)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock(instance.mutex);
        auto cbPtr = std::make_shared<EventCallback>(std::move(callback));
        instance.listener = cbPtr;
        return cbPtr;
    }

    // Unsubscribe from the event bus (remove all handlers)
    static void unsubscribe()
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock(instance.mutex);
        instance.listener.reset();  // Clear the stored callback
    }

    // Publish an event to the subscriber
    static void publish(int param, int value)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock(instance.mutex);

        if (instance.listener && *instance.listener)  // Check if the callback pointer and its target are valid
        {
            (*instance.listener)(param, value);  // Invoke the callback
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
    CallbackPtr listener;  // Store a single shared_ptr for the listener
    mutable std::mutex mutex;  // Mutex to protect access to listener
};
