#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_TextLines.h"
#include "ultra-shared/UI/UI_Helpers.h"

#include "UI/ui-colors.h"

//-----------------------------------------------------------------------------

GUI_TextLines::GUI_TextLines ( const juce::String& name, const Style _style, const juce::Justification _justification )
	: juce::Component ( name )
	, style ( _style )
	, justification ( _justification )
{
	setInterceptsMouseClicks ( false, false );
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setLines ( const juce::StringArray& newLines )
{
	if ( lines == newLines )
		return;

	lines = newLines;

	// The height follows the text even when the width stays
	resized ();
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setHighlights ( const juce::StringArray& parts )
{
	if ( highlights == parts )
		return;

	highlights = parts;
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setNote ( const juce::String& newNote )
{
	if ( note == newNote )
		return;

	note = newNote;
	resized ();
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setStyle ( const Style newStyle )
{
	if ( style == newStyle )
		return;

	style = newStyle;
	resized ();
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setAutoHeight ( const bool shouldSize )
{
	autoHeight = shouldSize;
	resized ();
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_TextLines::setSingleLine ( const bool single )
{
	singleLine = single;
	repaint ();
}
//-----------------------------------------------------------------------------

juce::Font GUI_TextLines::font () const
{
	switch ( style )
	{
		case Style::title:		return UI::fontSized ( 13.0f, 800 );
		case Style::header:		return UI::font ( UI::fonts::header_text );
		case Style::heading:	return UI::fontSized ( 15.0f, 800 );
		case Style::body:
		case Style::muted:
		case Style::error:		break;
	}

	return bodyFont ();
}
//-----------------------------------------------------------------------------

juce::Font GUI_TextLines::bodyFont ()
{
	return UI::fontSized ( 14.0f, 600 );
}
//-----------------------------------------------------------------------------

juce::Colour GUI_TextLines::colour () const
{
	switch ( style )
	{
		case Style::heading:	return findColour ( UI::colors::accent );
		case Style::muted:		return findColour ( UI::colors::textMuted );
		case Style::error:		return findColour ( UI::colors::statusError );
		case Style::title:
		case Style::header:
		case Style::body:		break;
	}

	return findColour ( UI::colors::text );
}
//-----------------------------------------------------------------------------

juce::TextLayout GUI_TextLines::wrapped () const
{
	juce::AttributedString	text;
	text.setJustification ( justification );
	const auto	all = lines.joinIntoString ( "\n" );

	std::vector<std::pair<int, int>>	spans;

	for ( const auto& part : highlights )
		if ( const auto at = all.indexOf ( part ); part.isNotEmpty () && at >= 0 )
			spans.emplace_back ( at, at + part.length () );

	std::ranges::sort ( spans );

	auto	pos = 0;

	for ( const auto& [ start, end ] : spans )
	{
		if ( start < pos )
			continue;

		text.append ( all.substring ( pos, start ), font (), colour () );
		text.append ( all.substring ( start, end ), font (), findColour ( UI::colors::text ) );
		pos = end;
	}

	text.append ( all.substring ( pos ), font (), colour () );

	if ( note.isNotEmpty () )
		text.append ( "  " + note, bodyFont (), findColour ( UI::colors::textMuted ) );

	juce::TextLayout	layout;
	layout.createLayout ( text, float ( getWidth () ) );

	return layout;
}
//-----------------------------------------------------------------------------

void GUI_TextLines::paint ( juce::Graphics& g )
{
	if ( lines.isEmpty () )
		return;

	if ( singleLine )
	{
		g.setFont ( font () );
		g.setColour ( colour () );
		g.drawText ( lines.joinIntoString ( "  " ), getLocalBounds (), justification, true );
		return;
	}

	wrapped ().draw ( g, getLocalBounds ().toFloat () );
}
//-----------------------------------------------------------------------------

void GUI_TextLines::resized ()
{
	if ( ! autoHeight )
		return;

	const auto	height = lines.isEmpty () ? 0 : int ( std::ceil ( wrapped ().getHeight () ) );

	if ( height != getHeight () )
		setSize ( getWidth (), height );
}
//-----------------------------------------------------------------------------
