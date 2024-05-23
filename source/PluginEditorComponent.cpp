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

    midiInputSelector.setBounds (inner.removeFromLeft (140));
    inner.removeFromLeft (7);
    midiOutputSelector.setBounds (inner.removeFromLeft (140));
    inner.removeFromLeft (7);
    midiChannelSelector.setBounds (inner.removeFromLeft (80));
    inner.removeFromLeft (7);
    electraSlotSelector.setBounds (inner.removeFromLeft (80));
    inner.removeFromLeft (7);
    ejectButton.setBounds (inner.removeFromRight (50));

    if (editor)
    {
        editor->setBounds (area);
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
