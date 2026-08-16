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
	// own pace with no timer behind it
	float	animPosition = 0.0f;
	float	animStartPosition = 0.0f;
	double	animStartTime = 0.0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Toggle )
};
//-----------------------------------------------------------------------------
