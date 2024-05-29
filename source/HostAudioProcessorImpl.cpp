#include "EventBus.h"
#include "HostAudioProcessorImpl.h"
#include "EditorTools.h"
#include "MidiTools.h"
#include "NRPNReceiver.h"
#include "ParameterChangeListener.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_core/juce_core.h"
#include <memory>

HostAudioProcessorImpl::HostAudioProcessorImpl()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    deviceManager.initialise (2, 2, nullptr, true);

    appProperties.setStorageParameters ([&] {
        juce::PropertiesFile::Options opt;
        opt.applicationName = getName();
        opt.commonToAllUsers = false;
        opt.doNotSave = false;
        opt.filenameSuffix = ".props";
        opt.ignoreCaseOfKeyNames = false;
        opt.storageFormat = juce::PropertiesFile::StorageFormat::storeAsXML;
        opt.osxLibrarySubFolder = "Application Support";
        return opt;
    }());

    pluginFormatManager.addDefaultFormats();

    if (auto savedPluginList = appProperties.getUserSettings()->getXmlValue ("pluginList"))
        pluginList.recreateFromXml (*savedPluginList);

    // load the parameters from the config if available
    if (auto parametersEl = appProperties.getUserSettings()->getXmlValue ("params"))
    {
        for (int i = 0; i < parametersEl->getNumChildElements(); i++)
        {
            auto parameterEl = parametersEl->getChildElement (i);
            auto name = parameterEl->getStringAttribute ("name");
            auto label = parameterEl->getStringAttribute ("label");
            // auto min = parameterEl->getDoubleAttribute("min");
            // auto max = parameterEl->getDoubleAttribute("max");
            auto def = parameterEl->getDoubleAttribute ("default");

            auto param = new juce::AudioParameterFloat (i, name, juce::NormalisableRange<float> (static_cast<float> (0), static_cast<float> (1)), static_cast<float> (def));

            // inform the wrapped plugin of external changes coming from the VST host
            auto newListener = new ParameterChangeListener (
                [&] (int index, float newValue) {
                    if (inner != nullptr && !isUpdatingParam)
                    {
                        isUpdatingParam = true;
                        // inform the wrapped plugin of external changes from the VST host
                        auto innerParam = inner->getParameters()[index];
                        innerParam->beginChangeGesture();
                        innerParam->setValue (newValue);
                        innerParam->endChangeGesture();
                        isUpdatingParam = false;
                        // publish the event that a parameter has changed
                        auto intValue = static_cast<int>(127 * 127 * newValue);
                        ParameterEventBus::publish(index, intValue);
                        // also inform the Electra One
                        if (midiOutput != nullptr)
                        {
                            sendOutgoingNRPN(index, intValue);
                        }
                    }
                });

            param->addListener (newListener);
            addParameter (param);
        }
    }

    juce::MessageManagerLock lock;
    pluginList.addChangeListener (this);
}

bool HostAudioProcessorImpl::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput = layouts.getMainInputChannelSet();

    if (!mainInput.isDisabled() && mainInput != mainOutput)
        return false;

    if (mainOutput.size() > 2)
        return false;

    return true;
}

void HostAudioProcessorImpl::prepareToPlay (double sr, int bs)
{
    const juce::ScopedLock sl (innerMutex);

    active = true;

    if (inner != nullptr)
    {
        inner->setRateAndBufferSizeDetails (sr, bs);
        inner->prepareToPlay (sr, bs);
    }
}

void HostAudioProcessorImpl::releaseResources()
{
    const juce::ScopedLock sl (innerMutex);

    // Save all parameters, so we can see them the next time the processor is loaded
    if (auto settings = appProperties.getUserSettings())
    {
        if (inner != nullptr)
        {
            auto params = inner->getParameters();
            auto paramsEl = new juce::XmlElement ("params");

            for (int i = 0; i < params.size(); i++)
            {
                auto param = params[i];
                auto paramEl = new juce::XmlElement ("param");
                paramEl->setAttribute ("name", param->getName (128));
                paramEl->setAttribute ("label", param->getLabel());
                paramEl->setAttribute ("default", param->getDefaultValue());
                paramEl->setAttribute ("steps", param->getNumSteps());
                paramsEl->addChildElement (paramEl);
            }

            settings->setValue ("params", paramsEl);
            settings->saveIfNeeded();
        }
    }

    active = false;

    if (inner != nullptr)
        inner->releaseResources();
}

void HostAudioProcessorImpl::reset()
{
    const juce::ScopedLock sl (innerMutex);

    if (inner != nullptr)
        inner->reset();
}

void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const juce::ScopedLock sl (innerMutex);

    if (active && inner != nullptr)
    {
        // Process block for loaded VST plugin
        inner->processBlock (buffer, midiMessages);
    }
}

void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages)
{
    const juce::ScopedLock sl (innerMutex);

    if (active && inner != nullptr)
    {
        // Assuming your inner plugin supports double precision processing:
        // Note that not all plugins support double precision; you may need a fallback or conversion.
        jassert (isUsingDoublePrecision()); // This line ensures that double precision is supported.

        // Create a temporary float buffer from the double buffer if needed or directly use as below:
        inner->processBlock (buffer, midiMessages);
    }
}

void HostAudioProcessorImpl::getStateInformation (juce::MemoryBlock& destData)
{
    const juce::ScopedLock sl (innerMutex);

    juce::XmlElement xml ("state");

    if (inner != nullptr)
    {
        xml.setAttribute (editorStyleTag, (int) editorStyle);

        xml.addChildElement (inner->getPluginDescription().createXml().release());

        xml.addChildElement ([this] {
            juce::MemoryBlock innerState;
            inner->getStateInformation (innerState);

            auto stateNode = std::make_unique<juce::XmlElement> (innerStateTag);
            stateNode->addTextElement (innerState.toBase64Encoding());
            return stateNode.release();
        }());

        auto midiNode = new juce::XmlElement ("midi");
        if (midiInputDeviceID.isNotEmpty())
        {
            midiNode->setAttribute ("in", midiInputDeviceID);
        }
        if (midiOutputDeviceID.isNotEmpty())
        {
            midiNode->setAttribute ("out", midiOutputDeviceID);
        }
        if (midiChannelID > 0)
        {
            midiNode->setAttribute ("channel", midiChannelID);
        }
        if (presetSlotID > 0)
        {
            midiNode->setAttribute ("slot", presetSlotID);
        }
        xml.addChildElement (midiNode);
    }

    const auto text = xml.toString();
    destData.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

void HostAudioProcessorImpl::setStateInformation (const void* data, int sizeInBytes)
{
    logToFile ("setStateInformation");
    const juce::ScopedLock sl (innerMutex);

    auto xml = juce::XmlDocument::parse (juce::String (juce::CharPointer_UTF8 (static_cast<const char*> (data)), (size_t) sizeInBytes));

    if (auto* pluginNode = xml->getChildByName ("PLUGIN"))
    {
        juce::PluginDescription pd;
        pd.loadFromXml (*pluginNode);

        juce::MemoryBlock innerState;
        innerState.fromBase64Encoding (xml->getChildElementAllSubText (innerStateTag, {}));

        setNewPlugin (pd,
            (EditorStyle) xml->getIntAttribute (editorStyleTag, 0),
            innerState);

        if (auto midiNode = xml->getChildByName ("midi"))
        {
            auto in = midiNode->getStringAttribute ("in");
            logToFile ("input >" + in + "<");
            if (in.isNotEmpty())
            {
                logToFile ("input not empty, setting");
                setMidiInput (in);
            }
            else
            {
                logToFile ("midi in not set");
            }

            auto out = midiNode->getStringAttribute ("out");
            logToFile ("output >" + out + "<");
            if (out.isNotEmpty())
            {
                logToFile ("output not empty, setting");
                setMidiOutput (out);
            }
            else
            {
                logToFile ("midi out not set");
            }

            auto channel = midiNode->getIntAttribute ("channel");
            if (channel > 0)
            {
                logToFile ("using channel " + static_cast<juce::String> (channel));
                midiChannelID = channel;
            }

            auto slot = midiNode->getIntAttribute ("slot");
            if (slot > 0)
            {
                logToFile ("using slot" + static_cast<juce::String> (slot));
                presetSlotID = slot;
            }
        }
    }
}

void HostAudioProcessorImpl::setMidiInput (juce::String deviceID)
{
    logToFile ("setting midi input to " + deviceID);
    if (midiInputDeviceID.isNotEmpty())
    {
        deviceManager.setMidiInputDeviceEnabled (midiInputDeviceID, false);
        deviceManager.removeMidiInputDeviceCallback (midiInputDeviceID, this);
    }

    deviceManager.setMidiInputDeviceEnabled (deviceID, true);
    deviceManager.addMidiInputDeviceCallback (deviceID, this);
    midiReceiver = std::make_unique<NRPNReceiver> (1, [this] (int param, int value) {
        handleIncomingNRPN (param, value);
    });

    midiInputDeviceID = deviceID;
}

void HostAudioProcessorImpl::setMidiOutput (juce::String deviceID)
{
    logToFile ("setting midi output to " + deviceID);
    if (midiOutputDeviceID.isNotEmpty() && midiOutput != nullptr)
    {
        midiOutput->clearAllPendingMessages();
    }

    auto outDevice = juce::MidiOutput::openDevice (deviceID);
    if (outDevice == nullptr)
    {
        logToFile ("midi out device not found");
        return;
    }

    logToFile ("midi out device found and created");
    logToFile (outDevice->getName());
    midiOutput = std::move (outDevice);
    midiOutputDeviceID = deviceID;
}

void HostAudioProcessorImpl::setNewPlugin (const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb)
{
    const juce::ScopedLock sl (innerMutex);

    const auto callback = [this, where, mb] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error) {
        if (error.isNotEmpty())
        {
            auto options = juce::MessageBoxOptions::makeOptionsOk (juce::MessageBoxIconType::WarningIcon,
                "Plugin Load Failed",
                error);
            messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
            return;
        }

        inner = std::move (instance);
        editorStyle = where;

        if (inner != nullptr && !mb.isEmpty())
            inner->setStateInformation (mb.getData(), (int) mb.getSize());

        // In a 'real' plugin, we'd also need to set the bus configuration of the inner plugin.
        // One possibility would be to match the bus configuration of the wrapper plugin, but
        // the inner plugin isn't guaranteed to support the same layout. Alternatively, we
        // could try to apply a reasonably similar layout, and maintain a mapping between the
        // inner/outer channel layouts.
        //
        // In any case, it is essential that the inner plugin is told about the bus
        // configuration that will be used. The AudioBuffer passed to the inner plugin must also
        // exactly match this layout.

        if (active)
        {
            inner->setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());
            inner->prepareToPlay (getSampleRate(), getBlockSize());

            // get all the parameters from the inner plugin
            pluginParams = inner->getParameters();

            // listen to inner plugin changes
            inner->addListener (this);
        }

        juce::NullCheckedInvocation::invoke (pluginChanged);
    };

    pluginFormatManager.createPluginInstanceAsync (pd, getSampleRate(), getBlockSize(), callback);
}

/** Receives a callback when an inner plugin parameter is changed.

    IMPORTANT NOTE: This will be called synchronously when a parameter changes, and
    many audio processors will change their parameter during their audio callback.
    This means that not only has your handler code got to be completely thread-safe,
    but it's also got to be VERY fast, and avoid blocking. If you need to handle
    this event on your message thread, use this callback to trigger an AsyncUpdater
    or ChangeBroadcaster which you can respond to on the message thread.
*/
void HostAudioProcessorImpl::audioProcessorParameterChanged (juce::AudioProcessor*,
    int parameterIndex,
    float newValue)
{
    logToFile ("incoming param change " + juce::String (parameterIndex) + " - " + juce::String (newValue));
    auto params = this->getParameters();
    if (parameterIndex < params.size() && !isUpdatingParam)
    {
        isUpdatingParam = true;

        // Ensure this code runs on the message thread
        // juce::MessageManager::callAsync ([this, params, parameterIndex, newValue]() {
        auto* param = params[parameterIndex];

        param->beginChangeGesture();
        // logToFile ("sending param change " + juce::String (parameterIndex) + " - " + juce::String (newValue));

        // Notify host about the change.
        // param->setValue (newValue);
        param->setValueNotifyingHost (newValue);
        // param->sendValueChangedMessageToListeners (newValue);

        param->endChangeGesture();

        // publish the event that a parameter has changed
        auto intValue = static_cast<int>(127 * 127 * newValue);
        ParameterEventBus::publish(parameterIndex, intValue);

        // also inform the Electra One
        sendOutgoingNRPN (parameterIndex, intValue);

        isUpdatingParam = false; // Ensure this flag is reset within the lambda
        // });
    }
}

void HostAudioProcessorImpl::audioProcessorChanged (AudioProcessor*, const ChangeDetails&) {}

void HostAudioProcessorImpl::clearPlugin()
{
    const juce::ScopedLock sl (innerMutex);

    inner->removeListener (this);
    inner = nullptr;
    pluginParams = nullptr;

    if (auto settings = appProperties.getUserSettings())
    {
        settings->removeValue ("params");
        settings->saveIfNeeded();
    }

    juce::NullCheckedInvocation::invoke (pluginChanged);
}

void HostAudioProcessorImpl::globalFocusChanged (juce::Component*) {}

bool HostAudioProcessorImpl::isPluginLoaded() const
{
    const juce::ScopedLock sl (innerMutex);
    return inner != nullptr;
}

std::unique_ptr<juce::AudioProcessorEditor> HostAudioProcessorImpl::createInnerEditor() const
{
    const juce::ScopedLock sl (innerMutex);
    return rawToUniquePtr (inner->hasEditor() ? inner->createEditorIfNeeded() : nullptr);
}

void HostAudioProcessorImpl::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source != &pluginList)
        return;

    if (auto savedPluginList = pluginList.createXml())
    {
        appProperties.getUserSettings()->setValue ("pluginList", savedPluginList.get());
        appProperties.saveIfNeeded();
    }
}

void HostAudioProcessorImpl::handleIncomingMidiMessage (juce::MidiInput* source,
    const juce::MidiMessage& message)
{
    logToFile ("incoming midi message");
    if (midiReceiver != nullptr)
    {
        // receiver will call handleIncomingNRPN
        midiReceiver->handleIncomingMidiMessage (source, message);
    }
}

void HostAudioProcessorImpl::handleIncomingNRPN (int parameterIndex, int value)
{
    logToFile ("incoming nrpm: " + static_cast<juce::String> (parameterIndex) + " - " + static_cast<juce::String> (value));

    int floor = static_cast<int> (std::floor (static_cast<float> (parameterIndex - 1) / static_cast<float> (MAX_PRESET_PARAMS)));
    int base = floor * MAX_PRESET_PARAMS;
    int parameter = parameterIndex - base - 1;
    float newValue = static_cast<float> (value) / 127 / 127;

    // publish the event that a parameter has changed
    auto intValue = static_cast<int>(127 * 127 * newValue);
    ParameterEventBus::publish(parameterIndex - 1, intValue);

    auto params = this->getParameters();
    if (parameter < params.size() && !isUpdatingParam)
    {
        isUpdatingParam = true;

        // Ensure this code runs on the message thread
        juce::MessageManager::callAsync ([this, params, parameter, newValue]() {
            auto* param = params[parameter];

            // logToFile ("sending param change " + juce::String (parameterIndex) + " - " + juce::String (newValue));
            param->beginChangeGesture();
            param->setValueNotifyingHost (newValue);
            param->endChangeGesture();

            auto innerParam = inner->getParameters()[parameter];
            innerParam->beginChangeGesture();
            innerParam->setValueNotifyingHost (newValue);
            innerParam->endChangeGesture();

            isUpdatingParam = false; // Ensure this flag is reset within the lambda
        });
    }
}

void HostAudioProcessorImpl::sendOutgoingNRPN (int parameter, int value)
{
    if (midiOutput != nullptr && midiChannelID > 0 && presetSlotID > 0)
    {
        int slotParamId = (presetSlotID - 1) * MAX_PRESET_PARAMS + parameter + 1;
        sendNRPN (midiOutput.get(), midiChannelID, slotParamId, value);
    }
}

void HostAudioProcessorImpl::handlePartialSysexMessage (juce::MidiInput*,
    const juce::uint8*,
    int,
    double)
{
    logToFile ("incoming sysex message");
}
