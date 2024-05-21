#pragma once

#include <mach-o/dyld.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>

class NRPNReceiver : public juce::MidiInputCallback
{
public:
    using HandlerType = std::function<void(int parameter, int value)>;

    NRPNReceiver(int channel, HandlerType handler)
        : midiChannel(channel - 1), nrpnHandler(handler), currentParameter(-1), currentValue(-1)
    {
        jassert(channel >= 1 && channel <= 16); // Ensure valid channel range
    }

    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage& message) override
    {
        if (message.isController())
        {
            int controllerNumber = message.getControllerNumber();
            int controllerValue = message.getControllerValue();

            if (message.getChannel() == midiChannel + 1)
            {
                switch (controllerNumber)
                {
                    case 99: // NRPN MSB
                        parameterMSB = controllerValue;
                        break;
                    case 98: // NRPN LSB
                        parameterLSB = controllerValue;
                        currentParameter = (parameterMSB << 7) | parameterLSB;
                        break;
                    case 6: // Data Entry MSB
                        valueMSB = controllerValue;
                        currentValue = (valueMSB << 7);
                        if (currentParameter != -1 && currentValue != -1)
                            nrpnHandler(currentParameter, currentValue);
                        break;
                    case 38: // Data Entry LSB
                        valueLSB = controllerValue;
                        if (currentParameter != -1 && valueMSB != -1)
                            nrpnHandler(currentParameter, (valueMSB << 7) | valueLSB);
                        break;
                }
            }
        }
    }

private:
    int midiChannel;

    HandlerType nrpnHandler;

    int parameterMSB{0};
    int parameterLSB{0};
    
    int valueMSB{0};
    int valueLSB{0};

    int currentParameter{-1};
    int currentValue{-1};
};
