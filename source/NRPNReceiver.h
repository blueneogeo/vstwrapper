#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

class NRPNReceiver : public juce::MidiInputCallback
{
public:
    using HandlerType = std::function<void (int parameter, int value)>;

    NRPNReceiver (int channel, HandlerType handler)
        : midiChannel (channel - 1), nrpnHandler (handler)
    {
        jassert (channel >= 1 && channel <= 16); // Ensure valid channel range
    }

    void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage& message) override
    {
        if (message.isController())
        {
            int controllerNumber = message.getControllerNumber();
            int controllerValue = message.getControllerValue();

            if (message.getChannel() == midiChannel + 1)
            {
                switch (controllerNumber)
                {
                    case 99: // NRPN Parameter MSB
                        parameterMSB = controllerValue;
                        break;
                    case 98: // NRPN Parameter LSB
                        parameterLSB = controllerValue;
                        currentParameter = (parameterMSB << 7) | parameterLSB;
                        break;
                    case 6: // Data Entry MSB
                        valueMSB = controllerValue;
                        currentValue = (valueMSB << 7); // Initialize with MSB part only
                        break;
                    case 38: // Data Entry LSB
                        valueLSB = controllerValue;
                        currentValue |= valueLSB; // Combine with LSB part

                        if (currentParameter != -1)
                            nrpnHandler (currentParameter, currentValue);

                        // Reset after handling complete NRPN message
                        resetState();
                        break;

                    default:
                        break; // Handle other controllers if necessary
                }
            }
        }
    }

private:
    void resetState()
    {
        currentParameter = -1;
        currentValue = -1;
        parameterMSB = 0;
        parameterLSB = 0;
        valueMSB = 0;
        valueLSB = 0;
    }

    int midiChannel;

    HandlerType nrpnHandler;

    int parameterMSB { 0 };
    int parameterLSB { 0 };

    int valueMSB { 0 };
    int valueLSB { 0 };

    int currentParameter { -1 };
    int currentValue { -1 };
};
