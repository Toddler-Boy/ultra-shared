#pragma once

#include <JuceHeader.h>

#include <optional>

#include "UI/ui-corners.h"
#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

// Inverted text button: solid, slightly rounded light background with dark
// text (the regular TextButton LAF style doesn't fit everywhere)

class GUI_SolidButton final : public juce::Button
{
public:
	GUI_SolidButton ( const juce::String& buttonName, const juce::String& stringsKey );

	// Another string of the strings file as its label
	void setTextKey ( const juce::String& key )	{	textKey = key;	repaint ();	}

	// Fills the "{}" in the string ("Update all ({})")
	void setArgument ( const juce::String& text )	{	argument = text;	repaint ();	}

	// A themed corner instead of the fixed one (a large radius gives a pill)
	void setCornerRole ( const UI::corners::Role role )	{	cornerRole = role;	repaint ();	}

	// The label's width at this button height, without padding
	[[ nodiscard ]] int textWidth ( float height ) const;

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isHover, bool isDown ) override;

	// juce::Component
	juce::MouseCursor getMouseCursor () override	{	return isEnabled () ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::ParentCursor;	}
	void enablementChanged () override				{	setAlpha ( isEnabled () ? 1.0f : 0.5f );	}

private:
	[[ nodiscard ]] juce::String label () const;

	juce::String	textKey;
	juce::String	argument;
	std::optional<UI::corners::Role>	cornerRole;

	juce::SharedResourcePointer<Strings>	strings;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SolidButton )
};
//-----------------------------------------------------------------------------
