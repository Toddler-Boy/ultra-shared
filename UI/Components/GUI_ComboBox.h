#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

class GUI_ComboBox final : public juce::ComboBox
{
public:
	explicit GUI_ComboBox ( const juce::String& componentName = {} )
		: juce::ComboBox ( componentName )	{}

	juce::MouseCursor getMouseCursor () override {	return isEnabled () ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor;	}
	void enablementChanged () override			{	setAlpha ( isEnabled () ? 1.0f : 0.35f );	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ComboBox )
};
//-----------------------------------------------------------------------------
