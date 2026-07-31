#pragma once

#include <JuceHeader.h>

#if ! JUCE_MAC

//-----------------------------------------------------------------------------

// Screen-covering fatal-error display, shown when the Data.pak (or naked
// Data folder) is missing or damaged: no fonts, strings or icons are
// reachable then, so it draws a bitmap compiled into the exe and swallows
// all input. The user can only close the app. Windows/Linux only - a macOS
// bundle with missing files fails its signature check and never launches
class GUI_FatalError final : public juce::Component
{
public:
	GUI_FatalError ();

	// juce::Component
	void paint ( juce::Graphics& g ) override;

private:
	juce::Image		bitmap;
	juce::Colour	bgCol { juce::Colours::black };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_FatalError )
};
//-----------------------------------------------------------------------------

#endif
