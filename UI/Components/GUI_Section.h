#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// A rounded box a shade lighter than the dialog, grouping a heading and what belongs to it

class GUI_Section final : public juce::Component
{
public:
	explicit GUI_Section ( const juce::String& name )
		: juce::Component ( name )
	{
	}

	// juce::Component
	void paint ( juce::Graphics& g ) override;

private:
	static constexpr float	corner = 6.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Section )
};
//-----------------------------------------------------------------------------
