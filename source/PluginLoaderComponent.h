#pragma once

#include "EditorTools.h"
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>
#include <juce_audio_processors/juce_audio_processors.h>

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
            grid.items = {
                juce::GridItem { thisWindowButton },
            };

            grid.performLayout (vertical.items[1].currentBounds.toNearestInt());
        }

        juce::Label label { "", "Select a plugin from the list, then display it using the buttons below." };
        juce::TextButton thisWindowButton { "Open VST3 Plugin" };
    };

    juce::PluginListComponent pluginListComponent;
    Buttons buttons;
};
