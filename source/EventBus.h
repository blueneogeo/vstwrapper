#pragma once

#include <functional>
#include <vector>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

class ParameterEventBus
{
public:
    using EventCallback = std::function<void (int param, int value)>;
    using CallbackPtr = std::shared_ptr<EventCallback>;

    static CallbackPtr subscribe (EventCallback callback)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);
        auto cbPtr = std::make_shared<EventCallback> (callback);
        instance.listeners.push_back (cbPtr);
        return cbPtr;
    }

    static void unsubscribe (CallbackPtr callbackPtr)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);

        auto it = std::remove_if (instance.listeners.begin(), instance.listeners.end(), [&callbackPtr] (const std::weak_ptr<EventCallback>& weakCb) {
            if (auto sharedCb = weakCb.lock())
            {
                return sharedCb == callbackPtr;
            }
            return false;
        });

        instance.listeners.erase (it, instance.listeners.end());
    }

    static void publish (int param, int value)
    {
        auto& instance = getInstance();
        std::lock_guard<std::mutex> lock (instance.mutex);

        for (auto& weakCb : instance.listeners)
        {
            if (auto cb = weakCb.lock())
            {
                (*cb) (param, value);
            }
        }
    }

private:
    ParameterEventBus() = default;

    static ParameterEventBus& getInstance()
    {
        static ParameterEventBus instance;
        return instance;
    }

    std::vector<std::weak_ptr<EventCallback>> listeners;
    std::mutex mutex;
};
