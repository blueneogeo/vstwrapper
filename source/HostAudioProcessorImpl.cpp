#include "HostAudioProcessorImpl.h"
#include "EditorTools.h"
#include "MidiTools.h"
#include "ParameterChangeListener.h"
#include "juce_audio_devices/juce_audio_devices.h"
#include "juce_core/juce_core.h"
#include <memory>

HostAudioProcessorImpl::HostAudioProcessorImpl()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
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
                        // also inform the Electra One
                        if (midiOutput != nullptr)
                        {
                            sendNRPN (midiOutput.get(), 1, index, static_cast<int> (newValue * 127 * 127));
                        }
                    }
                });

            param->addListener (newListener);
            addParameter (param);
        }
    }

    juce::MessageManagerLock lock;
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

        // Run parameters when processing block. Probably very bad idea, runs way too fast!
        // At least check if the values have changed!
        //
        // juce::MessageManager::callAsync ([this]() {
        //     for (auto param : getParameters())
        //     {
        //         if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
        //         {
        //             p->sendValueChangedMessageToListeners (p->getValue());
        //         }
        //     }
        // });
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

        // juce::MessageManager::callAsync ([this]() {
        //     for (auto param : getParameters())
        //     {
        //         if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
        //         {
        //             p->sendValueChangedMessageToListeners (p->getValue());
        //         }
        //     }
        // });
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
        if (midiInput != nullptr)
        {
            midiNode->setAttribute ("in", midiInput->getIdentifier());
        }
        if (midiOutput != nullptr)
        {
            midiNode->setAttribute ("out", midiOutput->getIdentifier());
        }
        xml.addChildElement (midiNode);
    }

    const auto text = xml.toString();
    destData.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

void HostAudioProcessorImpl::setStateInformation (const void* data, int sizeInBytes)
{
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
            auto out = midiNode->getStringAttribute ("out");

            if (in.isNotEmpty())
            {
                auto inDevice = juce::MidiInput::openDevice (in, this);
                if (inDevice == nullptr)
                {
                    logToFile ("midi in device not found");
                }
                else
                {
                    logToFile ("midi in device found and created");
                    logToFile (inDevice->getName());
                    midiInput = std::move (inDevice);
                }
            }
            if (out.isNotEmpty())
            {
                auto outDevice = juce::MidiOutput::openDevice(out);
                if (outDevice == nullptr)
                {
                    logToFile ("midi out device not found");
                }
                else
                {
                    logToFile ("midi out device found and created");
                    logToFile (outDevice->getName());
                    midiOutput = std::move (outDevice);
                }
            }
        }
    }
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

void HostAudioProcessorImpl::audioProcessorParameterChanged (juce::AudioProcessor*,
    int parameterIndex,
    float newValue)
{
    auto params = this->getParameters();
    if (parameterIndex < params.size() && !isUpdatingParam)
    {
        isUpdatingParam = true;

        // Ensure this code runs on the message thread
        juce::MessageManager::callAsync ([this, params, parameterIndex, newValue]() {
            auto* param = params[parameterIndex];

            param->beginChangeGesture();
            logToFile ("sending param change " + juce::String (parameterIndex) + " - " + juce::String (newValue));

            // Notify host about the change.
            // param->setValue (newValue);
            param->setValueNotifyingHost (newValue);
            // param->sendValueChangedMessageToListeners (newValue);

            param->endChangeGesture();

            // Verify if the value was set correctly.
            // float updatedValue = param->getValue();
            // logToFile ("sending param " + juce::String (param->getName (128)) + " - " + juce::String (updatedValue));

            // also inform the Electra One
            if (midiOutput != nullptr)
            {
                sendNRPN (midiOutput.get(), 1, parameterIndex, static_cast<int> (newValue * 127 * 127));
            }

            isUpdatingParam = false; // Ensure this flag is reset within the lambda
        });
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
    const juce::MidiMessage& message) {
    logToFile("incoming midi message");
    if(midiReceiver != nullptr) {
        // receiver will call handleIncomingNRPN
        midiReceiver->handleIncomingMidiMessage(source, message); }
    }

void HostAudioProcessorImpl::handleIncomingNRPN(int parameterIndex, int value)
{
    logToFile("incoming nrpm: " + static_cast<juce::String>(parameterIndex) + " - " + static_cast<juce::String>(value));

    float newValue = static_cast<float>(value) / 128 / 128;

    auto params = this->getParameters();
    if (parameterIndex < params.size() && !isUpdatingParam)
    {
        isUpdatingParam = true;

        // Ensure this code runs on the message thread
        juce::MessageManager::callAsync ([this, params, parameterIndex, newValue]() {
            auto* param = params[parameterIndex];

            param->beginChangeGesture();
            logToFile ("sending param change " + juce::String (parameterIndex) + " - " + juce::String (newValue));

            // Notify host about the change.
            // param->setValue (newValue);
            param->setValueNotifyingHost (newValue);
            // param->sendValueChangedMessageToListeners (newValue);

            param->endChangeGesture();

            auto innerParam = inner->getParameters()[parameterIndex];
            innerParam->beginChangeGesture();
            innerParam->setValueNotifyingHost(newValue);
            innerParam->endChangeGesture();

            isUpdatingParam = false; // Ensure this flag is reset within the lambda
        });
    }

}

void HostAudioProcessorImpl::handlePartialSysexMessage (juce::MidiInput*,
    const juce::uint8*,
    int,
    double) {
    logToFile("incoming sysex message");
}
