#pragma once
#include <JuceHeader.h>

// Hint texts for the hint line at the bottom of the window.
// The language changes the hint texts only. Default: Russian.
// The choice is a user setting for all instances:
//   %APPDATA%\desNotto\BerryTone\settings.xml
namespace hints
{
    enum class Lang { ru = 0, en = 1 };

    Lang getLang();
    void setLang (Lang);

    // moduleId: 0..6 = DSP modules (default order), 7 = Juice Level
    juce::String moduleText (Lang, int moduleId);
    // knobIndex: the position of the knob in its card (0..2)
    juce::String knobText (Lang, int moduleId, int knobIndex);
    // the text when the mouse is not over a module
    juce::String idleText (Lang);
}
