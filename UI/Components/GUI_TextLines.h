#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// Lines of text in one of the dialog text styles. A line too long for the width wraps, never squeezes. The text sits
// in the component's bounds as justified; with auto height the component sizes itself to the wrapped text

class GUI_TextLines final : public juce::Component
{
public:
	enum class Style
	{
		title,
		header,
		heading,
		body,
		muted,
		error,
	};

	GUI_TextLines ( const juce::String& name, Style style, juce::Justification justification = juce::Justification::centredLeft );

	// The body and muted styles' font, also the dialogs' text fields
	[[ nodiscard ]] static juce::Font bodyFont ();

	void setLines ( const juce::StringArray& newLines );
	void setText ( const juce::String& text )	{	setLines ( text.isEmpty () ? juce::StringArray {} : juce::StringArray { text } );	}

	// The first occurrence of each of these is drawn in the text colour, whatever the style
	void setHighlights ( const juce::StringArray& parts );

	// Muted body text after the last line; empty = none
	void setNote ( const juce::String& newNote );

	void setStyle ( Style newStyle );
	void setAutoHeight ( bool shouldSize );

	// One line, cut with an ellipsis where it runs out of width; highlights and the note are left out
	void setSingleLine ( bool single );

	// juce::Component
	void paint ( juce::Graphics& g ) override;
	void resized () override;

private:
	[[ nodiscard ]] juce::Font font () const;
	[[ nodiscard ]] juce::Colour colour () const;
	[[ nodiscard ]] juce::TextLayout wrapped () const;

	Style				style;
	juce::Justification	justification;
	juce::StringArray	lines;
	juce::StringArray	highlights;
	juce::String		note;
	bool				autoHeight = false;
	bool				singleLine = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_TextLines )
};
//-----------------------------------------------------------------------------
