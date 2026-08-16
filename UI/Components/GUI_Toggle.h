#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

class GUI_Toggle final : public juce::ToggleButton
{
public:
	GUI_Toggle ( const juce::String& name = {} );

	// juce::Component
	void enablementChanged () override;

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isHover, bool isDown ) override;
	void buttonStateChanged () override;

private:
	// The knob animation runs on wall time and keeps itself alive by asking
	// for the next repaint from within paint, so it moves at the display's
	// own pace with no timer behind it. The knob's two edges travel at
	// different speeds: the lead lands first and the tail catches up, which
	// stretches the knob into a pill while it moves
	float	animLead = 0.0f;
	float	animTail = 0.0f;
	float	leadStart = 0.0f;
	float	tailStart = 0.0f;
	double	animStartTime = 0.0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Toggle )
};
//-----------------------------------------------------------------------------
