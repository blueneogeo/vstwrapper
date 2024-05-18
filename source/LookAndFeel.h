
#include <juce_gui_basics/juce_gui_basics.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CustomLookAndFeel(float fontSize): comboFontSize(fontSize), buttonFontSize(fontSize) {}

    juce::Font getComboBoxFont(juce::ComboBox& comboBox) override
    {
        return juce::Font(comboFontSize);
    }

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override
    {
        return juce::Font(buttonFontSize);
    }
private:
  float comboFontSize;
  float buttonFontSize;
};
