#include "WrapperPlugin.h"

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
        }

        juce::NullCheckedInvocation::invoke (pluginChanged);
    };

    pluginFormatManager.createPluginInstanceAsync (pd, getSampleRate(), getBlockSize(), callback);
}

void HostAudioProcessorImpl::clearPlugin()
{
    const juce::ScopedLock sl (innerMutex);

    inner = nullptr;
    juce::NullCheckedInvocation::invoke (pluginChanged);
}

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
