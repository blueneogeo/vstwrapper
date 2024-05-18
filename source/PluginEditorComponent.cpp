#include "PluginEditorComponent.h"
#include "EditorTools.h"

void PluginEditorComponent::setScaleFactor (float scale)
{
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized()
{
    const int width = 150;
    
    auto area = getLocalBounds();
    
    auto toolbar = area.removeFromTop(toolbarHeight + 10).reduced(7, 5);
    auto inner = toolbar.removeFromTop(toolbarHeight);

    midiInputSelector.setBounds(inner.removeFromLeft(width));
    inner.removeFromLeft(7);
    midiOutputSelector.setBounds(inner.removeFromLeft(width));
    closeButton.setBounds(inner.removeFromRight(50));

    if(editor) {
        editor->setBounds(area);
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

void PluginEditorComponent::midiInputChanged()
{
    auto selectedIndex = midiInputSelector.getSelectedItemIndex();

    if (selectedIndex >= 0)
        DBG ("MIDI Input selected: " + juce::MidiInput::getAvailableDevices()[selectedIndex].name);

    // Handle MIDI input change logic here...
}

void PluginEditorComponent::midiOutputChanged()
{
    auto selectedIndex = midiOutputSelector.getSelectedItemIndex();

    if (selectedIndex >= 0)
        DBG ("MIDI Output Selected:" + juice ::Midi Output ::get Available Devices()[selected Index].name);

    // Handle MIDI output change logic here ...
}
