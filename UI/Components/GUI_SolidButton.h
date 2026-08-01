#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

// Inverted text button: solid, slightly rounded light background with dark
// text (the regular TextButton LAF style doesn't fit everywhere)

class GUI_SolidButton final : public juce::Button
{
public:
	GUI_SolidButton ( const juce::String& buttonName, const juce::String& stringsKey );

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isHover, bool isDown ) override;

	// juce::Component
	juce::MouseCursor getMouseCursor () override	{	return juce::MouseCursor::PointingHandCursor;	}

private:
	juce::String	textKey;

	juce::SharedResourcePointer<Strings>	strings;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SolidButton )
};
//-----------------------------------------------------------------------------
