#pragma once

#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
enum class EditorStyle { thisWindow,
    newWindow };

class HostAudioProcessorImpl : public juce::AudioProcessor,
                               private juce::ChangeListener
{
public:
    HostAudioProcessorImpl();

    bool isBusesLayoutSupported (const BusesLayout& layouts) const final;

    void prepareToPlay (double sr, int bs) final;

    void releaseResources() final;

    void reset() final;

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) final;

    void processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midiMessages) final;
    bool hasEditor() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }

    const juce::String getName() const final { return "HostPluginDemo"; }
    bool acceptsMidi() const final { return true; }
    bool producesMidi() const final { return true; }
    double getTailLengthSeconds() const final { return 0.0; }

    int getNumPrograms() final { return 0; }
    int getCurrentProgram() final { return 0; }
    void setCurrentProgram (int) final {}
    const juce::String getProgramName (int) final { return "None"; }
    void changeProgramName (int, const juce::String&) final {}

    void getStateInformation (juce::MemoryBlock& destData) final;

    void setStateInformation (const void* data, int sizeInBytes) final;

    void setNewPlugin (const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb = {});
    void clearPlugin();

    bool isPluginLoaded() const;

    std::unique_ptr<juce::AudioProcessorEditor> createInnerEditor() const;

    EditorStyle getEditorStyle() const noexcept { return editorStyle; }

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList pluginList;
    std::function<void()> pluginChanged;

private:
    juce::CriticalSection innerMutex;
    std::unique_ptr<juce::AudioPluginInstance> inner;
    EditorStyle editorStyle = EditorStyle {};
    bool active = false;
    juce::ScopedMessageBox messageBox;

    static constexpr const char* innerStateTag = "inner_state";
    static constexpr const char* editorStyleTag = "editor_style";

    void changeListenerCallback (juce::ChangeBroadcaster* source) final;
};

//==============================================================================
constexpr auto margin = 10;

static void doLayout (juce::Component* main, juce::Component& bottom, int bottomHeight, juce::Rectangle<int> bounds)
{
    juce::Grid grid;
    grid.setGap (juce::Grid::Px { margin });
    grid.templateColumns = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } } };
    grid.templateRows = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } },
        juce::Grid::TrackInfo { juce::Grid::Px { bottomHeight } } };
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

        const auto getCallback = [this, &list, cb = std::forward<Callback> (callback)] (EditorStyle style) {
            return [this, &list, cb, style] {
                const auto index = pluginListComponent.getTableListBox().getSelectedRow();
                const auto& types = list.getTypes();

                if (juce::isPositiveAndBelow (index, types.size()))
                    juce::NullCheckedInvocation::invoke (cb, types.getReference (index), style);
            };
        };

        buttons.thisWindowButton.onClick = getCallback (EditorStyle::thisWindow);
        buttons.newWindowButton.onClick = getCallback (EditorStyle::newWindow);
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
            vertical.items.insertMultiple (0, juce::GridItem {}, 2);
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
                  EditorStyle editorStyle) {
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

        [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([ref = SafePointer<HostAudioProcessorEditor> (this), scale] {
            if (auto* r = ref.getComponent())
                if (auto* e = r->currentEditorComponent)
                    e->setScaleFactor (scale);
        });

        jassert (posted);
    }

private:
    void pluginChanged()
    {
        loader.setVisible (!hostProcessor.isPluginLoaded());
        closeButton.setVisible (hostProcessor.isPluginLoaded());

        if (hostProcessor.isPluginLoaded())
        {
            auto editorComponent = std::make_unique<PluginEditorComponent> (hostProcessor.createInnerEditor(), [this] {
                [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([this] { clearPlugin(); });
                jassert (posted);
            });

            editorComponent->setScaleFactor (currentScaleFactor);
            currentEditorComponent = editorComponent.get();

            editor = [&]() -> std::unique_ptr<Component> {
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
