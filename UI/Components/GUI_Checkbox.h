#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// A square check box: accent fill with a check mark when on, dark box when off
class GUI_Checkbox final : public juce::ToggleButton
{
public:
	GUI_Checkbox ( const juce::String& name = {} );

	// juce::Component
	void enablementChanged () override;

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isHover, bool isDown ) override;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Checkbox )
};
//-----------------------------------------------------------------------------
