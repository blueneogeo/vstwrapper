/*
/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:                  HostPluginDemo
 version:               1.0.0
 vendor:                JUCE
 website:               http://juce.com
 description:           Plugin that can host other plugins

 dependencies:          juce_audio_basics, juce_audio_devices, juce_audio_formats,
                        juce_audio_plugin_client, juce_audio_processors,
                        juce_audio_utils, juce_core, juce_data_structures,
                        juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:             xcode_mac, vs2022, linux_make

 moduleFlags:           JUCE_STRICT_REFCOUNTEDPOINTER=1
                        JUCE_PLUGINHOST_LV2=1
                        JUCE_PLUGINHOST_VST3=1
                        JUCE_PLUGINHOST_VST=0
                        JUCE_PLUGINHOST_AU=1

 type:                  AudioProcessor
 mainClass:             HostAudioProcessor

 useLocalCopy:          1

 pluginCharacteristics: pluginIsSynth, pluginWantsMidiIn, pluginProducesMidiOut,
                        pluginEditorRequiresKeys

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

//==============================================================================
enum class EditorStyle { thisWindow, newWindow };

class HostAudioProcessorImpl : public juce::AudioProcessor,
                               private juce::ChangeListener
{
public:
    HostAudioProcessorImpl()
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                           .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    {
        appProperties.setStorageParameters ([&]
        {
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

    bool isBusesLayoutSupported (const BusesLayout& layouts) const final
    {
        const auto& mainOutput = layouts.getMainOutputChannelSet();
        const auto& mainInput  = layouts.getMainInputChannelSet();

        if (! mainInput.isDisabled() && mainInput != mainOutput)
            return false;

        if (mainOutput.size() > 2)
            return false;

        return true;
    }

    void prepareToPlay (double sr, int bs) final
    {
        const juce::ScopedLock sl (innerMutex);

        active = true;

        if (inner != nullptr)
        {
            inner->setRateAndBufferSizeDetails (sr, bs);
            inner->prepareToPlay (sr, bs);
        }
    }

    void releaseResources() final
    {
        const juce::ScopedLock sl (innerMutex);

        active = false;

        if (inner != nullptr)
            inner->releaseResources();
    }

    void reset() final
    {
        const juce::ScopedLock sl (innerMutex);

        if (inner != nullptr)
            inner->reset();
    }

    // In this example, we don't actually pass any audio through the inner processor.
    // In a 'real' plugin, we'd need to add some synchronisation to ensure that the inner
    // plugin instance was never modified (deleted, replaced etc.) during a call to processBlock.
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) final
    {
        jassert (! isUsingDoublePrecision());
    }

    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) final
    {
        jassert (isUsingDoublePrecision());
    }

    bool hasEditor() const override                                   { return false; }
    juce::AudioProcessorEditor* createEditor() override                     { return nullptr; }

    const juce::String getName() const final                                { return "HostPluginDemo"; }
    bool acceptsMidi() const final                                    { return true; }
    bool producesMidi() const final                                   { return true; }
    double getTailLengthSeconds() const final                         { return 0.0; }

    int getNumPrograms() final                                        { return 0; }
    int getCurrentProgram() final                                     { return 0; }
    void setCurrentProgram (int) final                                {}
    const juce::String getProgramName (int) final                           { return "None"; }
    void changeProgramName (int, const juce::String&) final                 {}

    void getStateInformation (juce::MemoryBlock& destData) final
    {
        const juce::ScopedLock sl (innerMutex);

        juce::XmlElement xml ("state");

        if (inner != nullptr)
        {
            xml.setAttribute (editorStyleTag, (int) editorStyle);
            xml.addChildElement (inner->getPluginDescription().createXml().release());
            xml.addChildElement ([this]
            {
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

    void setStateInformation (const void* data, int sizeInBytes) final
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

    void setNewPlugin (const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb = {})
    {
        const juce::ScopedLock sl (innerMutex);

        const auto callback = [this, where, mb] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
        {
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

            if (inner != nullptr && ! mb.isEmpty())
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

    void clearPlugin()
    {
        const juce::ScopedLock sl (innerMutex);

        inner = nullptr;
        juce::NullCheckedInvocation::invoke (pluginChanged);
    }

    bool isPluginLoaded() const
    {
        const juce::ScopedLock sl (innerMutex);
        return inner != nullptr;
    }

    std::unique_ptr<juce::AudioProcessorEditor> createInnerEditor() const
    {
        const juce::ScopedLock sl (innerMutex);
        return rawToUniquePtr (inner->hasEditor() ? inner->createEditorIfNeeded() : nullptr);
    }

    EditorStyle getEditorStyle() const noexcept { return editorStyle; }

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList pluginList;
    std::function<void()> pluginChanged;

private:
    juce::CriticalSection innerMutex;
    std::unique_ptr<juce::AudioPluginInstance> inner;
    EditorStyle editorStyle = EditorStyle{};
    bool active = false;
    juce::ScopedMessageBox messageBox;

    static constexpr const char* innerStateTag = "inner_state";
    static constexpr const char* editorStyleTag = "editor_style";

    void changeListenerCallback (juce::ChangeBroadcaster* source) final
    {
        if (source != &pluginList)
            return;

        if (auto savedPluginList = pluginList.createXml())
        {
            appProperties.getUserSettings()->setValue ("pluginList", savedPluginList.get());
            appProperties.saveIfNeeded();
        }
    }
};

//==============================================================================
constexpr auto margin = 10;

static void doLayout (juce::Component* main, juce::Component& bottom, int bottomHeight, juce::Rectangle<int> bounds)
{
    juce::Grid grid;
    grid.setGap (juce::Grid::Px { margin });
    grid.templateColumns = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } } };
    grid.templateRows = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } },
                          juce::Grid::TrackInfo { juce::Grid::Px { bottomHeight }} };
    grid.items = { juce::GridItem { main }, juce::GridItem { bottom }.withMargin ({ 0, margin, margin, margin }) };
    grid.performLayout (bounds);
}

class PluginLoaderComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginLoaderComponent (juce::AudioPluginFormatManager& manager,
                           juce::KnownPluginList& list,
                           Callback&& callback)
        : pluginListComponent (manager, list, {}, {})
    {
        pluginListComponent.getTableListBox().setMultipleSelectionEnabled (false);

        addAndMakeVisible (pluginListComponent);
        addAndMakeVisible (buttons);

        const auto getCallback = [this, &list, cb = std::forward<Callback> (callback)] (EditorStyle style)
        {
            return [this, &list, cb, style]
            {
                const auto index = pluginListComponent.getTableListBox().getSelectedRow();
                const auto& types = list.getTypes();

                if (juce::isPositiveAndBelow (index, types.size()))
                    juce::NullCheckedInvocation::invoke (cb, types.getReference (index), style);
            };
        };

        buttons.thisWindowButton.onClick = getCallback (EditorStyle::thisWindow);
        buttons.newWindowButton .onClick = getCallback (EditorStyle::newWindow);
    }

    void resized() override
    {
        doLayout (&pluginListComponent, buttons, 80, getLocalBounds());
    }

private:
    struct Buttons final : public Component
    {
        Buttons()
        {
            label.setJustificationType (juce::Justification::centred);

            addAndMakeVisible (label);
            addAndMakeVisible (thisWindowButton);
            addAndMakeVisible (newWindowButton);
        }

        void resized() override
        {
            juce::Grid vertical;
            vertical.autoFlow = juce::Grid::AutoFlow::row;
            vertical.setGap (juce::Grid::Px { margin });
            vertical.autoRows = vertical.autoColumns = juce::Grid::TrackInfo { juce::Grid::Fr { 1 } };
            vertical.items.insertMultiple (0, juce::GridItem{}, 2);
            vertical.performLayout (getLocalBounds());

            label.setBounds (vertical.items[0].currentBounds.toNearestInt());

            juce::Grid grid;
            grid.autoFlow = juce::Grid::AutoFlow::column;
            grid.setGap (juce::Grid::Px { margin });
            grid.autoRows = grid.autoColumns = juce::Grid::TrackInfo { juce::Grid::Fr { 1 } };
            grid.items = { juce::GridItem { thisWindowButton },
                           juce::GridItem { newWindowButton } };

            grid.performLayout (vertical.items[1].currentBounds.toNearestInt());
        }

        juce::Label label { "", "Select a plugin from the list, then display it using the buttons below." };
        juce::TextButton thisWindowButton { "Open In This Window" };
        juce::TextButton newWindowButton { "Open In New Window" };
    };

    juce::PluginListComponent pluginListComponent;
    Buttons buttons;
};

//==============================================================================
class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
        : editor (std::move (editorIn))
    {
        addAndMakeVisible (editor.get());
        addAndMakeVisible (closeButton);

        childBoundsChanged (editor.get());

        closeButton.onClick = std::forward<Callback> (onClose);
    }

    void setScaleFactor (float scale)
    {
        if (editor != nullptr)
            editor->setScaleFactor (scale);
    }

    void resized() override
    {
        doLayout (editor.get(), closeButton, buttonHeight, getLocalBounds());
    }

    void childBoundsChanged (Component* child) override
    {
        if (child != editor.get())
            return;

        const auto size = editor != nullptr ? editor->getLocalBounds()
                                            : juce::Rectangle<int>();

        setSize (size.getWidth(), margin + buttonHeight + size.getHeight());
    }

private:
    static constexpr auto buttonHeight = 40;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton closeButton { "Close Plugin" };
};

//==============================================================================
class ScaledDocumentWindow final : public juce::DocumentWindow
{
public:
    ScaledDocumentWindow (juce::Colour bg, float scale)
        : DocumentWindow ("Editor", bg, 0), desktopScale (scale) {}

    float getDesktopScaleFactor() const override { return juce::Desktop::getInstance().getGlobalScaleFactor() * desktopScale; }

private:
    float desktopScale = 1.0f;
};

//==============================================================================
class HostAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HostAudioProcessorEditor (HostAudioProcessorImpl& owner)
        : AudioProcessorEditor (owner),
          hostProcessor (owner),
          loader (owner.pluginFormatManager,
                  owner.pluginList,
                  [&owner] (const juce::PluginDescription& pd,
                            EditorStyle editorStyle)
                  {
                      owner.setNewPlugin (pd, editorStyle);
                  }),
          scopedCallback (owner.pluginChanged, [this] { pluginChanged(); })
    {
        setSize (500, 500);
        setResizable (false, false);
        addAndMakeVisible (closeButton);
        addAndMakeVisible (loader);

        hostProcessor.pluginChanged();

        closeButton.onClick = [this] { clearPlugin(); };
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker());
    }

    void resized() override
    {
        closeButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, buttonHeight));
        loader.setBounds (getLocalBounds());
    }

    void childBoundsChanged (Component* child) override
    {
        if (child != editor.get())
            return;

        const auto size = editor != nullptr ? editor->getLocalBounds()
                                            : juce::Rectangle<int>();

        setSize (size.getWidth(), size.getHeight());
    }

    void setScaleFactor (float scale) override
    {
        currentScaleFactor = scale;
        AudioProcessorEditor::setScaleFactor (scale);

        [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([ref = SafePointer<HostAudioProcessorEditor> (this), scale]
        {
            if (auto* r = ref.getComponent())
                if (auto* e = r->currentEditorComponent)
                    e->setScaleFactor (scale);
        });

        jassert (posted);
    }

private:
    void pluginChanged()
    {
        loader.setVisible (! hostProcessor.isPluginLoaded());
        closeButton.setVisible (hostProcessor.isPluginLoaded());

        if (hostProcessor.isPluginLoaded())
        {
            auto editorComponent = std::make_unique<PluginEditorComponent> (hostProcessor.createInnerEditor(), [this]
            {
                [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([this] { clearPlugin(); });
                jassert (posted);
            });

            editorComponent->setScaleFactor (currentScaleFactor);
            currentEditorComponent = editorComponent.get();

            editor = [&]() -> std::unique_ptr<Component>
            {
                switch (hostProcessor.getEditorStyle())
                {
                    case EditorStyle::thisWindow:
                        addAndMakeVisible (editorComponent.get());
                        setSize (editorComponent->getWidth(), editorComponent->getHeight());
                        return std::move (editorComponent);

                    case EditorStyle::newWindow:
                        const auto bg = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker();
                        auto window = std::make_unique<ScaledDocumentWindow> (bg, currentScaleFactor);
                        window->setAlwaysOnTop (true);
                        window->setContentOwned (editorComponent.release(), true);
                        window->centreAroundComponent (this, window->getWidth(), window->getHeight());
                        window->setVisible (true);
                        return window;
                }

                jassertfalse;
                return nullptr;
            }();
        }
        else
        {
            editor = nullptr;
            setSize (500, 500);
        }
    }

    void clearPlugin()
    {
        currentEditorComponent = nullptr;
        editor = nullptr;
        hostProcessor.clearPlugin();
    }

    static constexpr auto buttonHeight = 30;

    HostAudioProcessorImpl& hostProcessor;
    PluginLoaderComponent loader;
    std::unique_ptr<Component> editor;
    PluginEditorComponent* currentEditorComponent = nullptr;
    juce::ScopedValueSetter<std::function<void()>> scopedCallback;
    juce::TextButton closeButton { "Close Plugin" };
    float currentScaleFactor = 1.0f;
};

//==============================================================================
class HostAudioProcessor final : public HostAudioProcessorImpl
{
public:
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override { return new HostAudioProcessorEditor (*this); }
};


