#pragma once

#include "EditorTools.h"
#include "Images.h"
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

        auto svg = juce::parseXML (electraOneSVGLogo);
        svgLogo = juce::Drawable::createFromSVG (*svg);
        juce::Rectangle<float> targetBounds (0, 0, 60, 20);
        svgLogo->setTransformToFit (targetBounds, juce::RectanglePlacement::centred);

        addAndMakeVisible (svgLogo.get());
        addAndMakeVisible (pluginListComponent);
        addAndMakeVisible (openPluginButton);

        const auto getCallback = [this, &list, cb = std::forward<Callback> (callback)] (EditorStyle style) {
            return [this, &list, cb, style] {
                const auto index = pluginListComponent.getTableListBox().getSelectedRow();
                const auto& types = list.getTypes();

                if (juce::isPositiveAndBelow (index, types.size()))
                    juce::NullCheckedInvocation::invoke (cb, types.getReference (index), style);
            };
        };

        openPluginButton.onClick = getCallback (EditorStyle::thisWindow);
    }

    void resized() override
    {
        juce::Grid grid;
        grid.setGap (juce::Grid::Px { margin });
        grid.templateColumns = {
            juce::Grid::TrackInfo { juce::Grid::Fr { 1 } }
        };
        grid.templateRows = {
            juce::Grid::TrackInfo { juce::Grid::Px { 20 } },
            juce::Grid::TrackInfo { juce::Grid::Fr { 1 } },
            juce::Grid::TrackInfo { juce::Grid::Px { 40 } }
        };
        grid.items = {
            juce::GridItem { svgLogo.get() }.withMargin ({ margin * 2, margin * 2, margin * 2, margin * 2 }),
            juce::GridItem { pluginListComponent }.withMargin ({ margin, margin, 0, margin }),
            juce::GridItem { openPluginButton }.withMargin ({ 0, margin, margin, margin })
        };
        grid.performLayout (getLocalBounds());
    }

private:
    std::unique_ptr<juce::Drawable> svgLogo;
    juce::PluginListComponent pluginListComponent;
    juce::TextButton openPluginButton { "Open VST3 Plugin" };
};
