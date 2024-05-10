#include "PluginEditorComponent.h"
#include "EditorTools.h"

void PluginEditorComponent::setScaleFactor (float scale)
{
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized()
{
    doLayout (editor.get(), closeButton, buttonHeight, getLocalBounds());
}

void PluginEditorComponent::childBoundsChanged (Component* child)
{
    if (child != editor.get())
        return;

    const auto size = editor != nullptr ? editor->getLocalBounds()
                                        : juce::Rectangle<int>();

    setSize (size.getWidth(), margin + buttonHeight + size.getHeight());
}
