#include "PluginEditorComponent.h"
#include "EditorTools.h"
#include <memory>

void PluginEditorComponent::setScaleFactor (float scale)
{
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized()
{
    auto area = getLocalBounds();

    auto toolbar = area.removeFromTop (toolbarHeight + 10).reduced (7, 5);
    auto inner = toolbar.removeFromTop (toolbarHeight);

    // electraLabel.setBounds(inner.removeFromLeft(100));
    inner.removeFromLeft (80); // Electra One logo space
    midiInputSelector.setBounds (inner.removeFromLeft (140));
    inner.removeFromLeft (7);
    midiOutputSelector.setBounds (inner.removeFromLeft (140));
    inner.removeFromLeft (7);
    midiChannelSelector.setBounds (inner.removeFromLeft (100));
    inner.removeFromLeft (7);
    electraSlotSelector.setBounds (inner.removeFromLeft (100));
    ejectButton.setBounds (inner.removeFromRight (50));
    paramLabel.setBounds (inner.removeFromRight(40));

    if (editor)
    {
        editor->setBounds (area);
    }
}

void PluginEditorComponent::paint (juce::Graphics& g)
{
    if (svgLogo)
    {
        svgLogo->drawAt(g, 5, 5, 1);
    }
}

void PluginEditorComponent::childBoundsChanged (Component* child)
{
    if (child != editor.get())
        return;

    const auto size = editor != nullptr ? editor->getLocalBounds()
                                        : juce::Rectangle<int>();

    setSize (size.getWidth(), margin + toolbarHeight + size.getHeight());
}
