#include "HostAudioProcessorEditor.h"
#include "ScaledDocumentWindow.h"

HostAudioProcessorEditor::HostAudioProcessorEditor (HostAudioProcessorImpl& owner)
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
    addAndMakeVisible (loader);

    hostProcessor.pluginChanged();
}

void HostAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker());
}

void HostAudioProcessorEditor::resized()
{
    loader.setBounds (getLocalBounds());
}

void HostAudioProcessorEditor::childBoundsChanged (Component* child)
{
    if (child != editor.get())
        return;

    const auto size = editor != nullptr ? editor->getLocalBounds()
                                        : juce::Rectangle<int>();

    setSize (size.getWidth(), size.getHeight());
}

void HostAudioProcessorEditor::setScaleFactor (float scale)
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

void HostAudioProcessorEditor::pluginChanged()
{
    loader.setVisible (!hostProcessor.isPluginLoaded());

    if (hostProcessor.isPluginLoaded())
    {
        auto editorComponent = std::make_unique<PluginEditorComponent> (&hostProcessor,  hostProcessor.createInnerEditor(), [this] {
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

void HostAudioProcessorEditor::clearPlugin()
{
    currentEditorComponent = nullptr;
    editor = nullptr;
    hostProcessor.clearPlugin();
}
