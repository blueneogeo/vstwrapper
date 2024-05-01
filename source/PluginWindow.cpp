#include "PluginWindow.h"

PluginWindow::PluginWindow(juce::AudioProcessor* thePlugin)
    : DocumentWindow(thePlugin->getName(), juce::Colours::lightgrey, TitleBarButtons::closeButton)
    , plugin(thePlugin)
{
    auto pluginEditor = plugin->createEditorIfNeeded();
    editor.reset(pluginEditor);
    editor->setOpaque(true);
    setContentNonOwned(editor.get(), true);
    editor->setTopLeftPosition(1, 1 + getTitleBarHeight());

    setTopLeftPosition(200, 200);
    setAlwaysOnTop(true);
    setVisible(true);
}

void PluginWindow::closeButtonPressed()
{
    plugin->editorBeingDeleted(editor.get());
    editor = nullptr;
    sendChangeMessage();
}

PluginWindow::~PluginWindow()
{
    if (editor) closeButtonPressed();
}
