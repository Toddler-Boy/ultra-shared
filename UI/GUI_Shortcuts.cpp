#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_Shortcuts.h"

#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

namespace
{
	// The keycap look, resolved from the theme per paint so hot-reloads land
	struct CapStyle
	{
		float		height = UI::paddingDef ( UI::paddings::keycap_height ).top;
		float		gap = UI::paddingDef ( UI::paddings::keycap_gap ).top;
		float		pad = UI::paddingDef ( UI::paddings::keycap ).left;
		float		corner = UI::corner ( UI::corners::keycap );
		float		line = UI::lineWidth ( UI::lines::keycap );
		juce::Font	font = UI::font ( UI::fonts::keycap );

		[[ nodiscard ]] float width ( const shortcuts::Cap& cap ) const
		{
			if ( cap.svg.isNotEmpty () )
				return height;

			return std::max ( height, std::ceil ( juce::GlyphArrangement::getStringWidth ( font, cap.text ) ) + pad * 2.0f );
		}
	};

	void drawCap ( juce::Graphics& g, const juce::Component& owner, const CapStyle& style, const shortcuts::Cap& cap, const juce::Rectangle<float>& r )
	{
		g.setColour ( owner.findColour ( UI::colors::keycapFill ) );
		g.fillRoundedRectangle ( r, style.corner );

		if ( UI::lines::visible ( style.line ) )
		{
			g.setColour ( owner.findColour ( UI::colors::keycapOutline ) );
			g.drawRoundedRectangle ( r.reduced ( style.line * 0.5f ), style.corner, style.line );
		}

		g.setColour ( owner.findColour ( UI::colors::text ) );

		if ( cap.svg.isNotEmpty () )
		{
			// Scaled at the origin so the path cache hits for every cap
			const auto	symbol = r.reduced ( style.pad ).withZeroOrigin ();
			const auto&	p = UI::getScaledPath ( cap.svg, symbol, juce::RectanglePlacement::centred );

			g.fillPath ( p, juce::AffineTransform::translation ( r.getX () + style.pad, r.getY () + style.pad ) );
		}
		else
		{
			g.setFont ( style.font );
			g.drawText ( cap.text, r, juce::Justification::centred, false );
		}
	}

	// Draws a chord's caps left to right from x, returns the x after them
	float drawChord ( juce::Graphics& g, const juce::Component& owner, const CapStyle& style, const shortcuts::Chord& chord, float x, const float centreY )
	{
		for ( const auto& cap : chord.caps )
		{
			const auto	w = style.width ( cap );

			drawCap ( g, owner, style, cap, { x, centreY - style.height * 0.5f, w, style.height } );
			x += w + style.gap;
		}

		return x - style.gap;
	}

	[[ nodiscard ]] float chordWidth ( const CapStyle& style, const shortcuts::Chord& chord )
	{
		auto	w = 0.0f;
		for ( const auto& cap : chord.caps )
			w += style.width ( cap ) + style.gap;

		return w - style.gap;
	}
}
//-----------------------------------------------------------------------------

GUI_ShortcutList::GUI_ShortcutList ()
{
	juce::String	section;

	for ( const auto& entry : shortcuts->getEntries () )
	{
		if ( entry.section != section )
		{
			section = entry.section;
			items.push_back ( { true, ( "shortcuts/section/" + section ).toLowerCase (), nullptr } );
		}

		items.push_back ( { false, ( "shortcuts/" + section + "/" + entry.verb ).toLowerCase (), &entry.chords } );
	}

	updateHeight ();
}
//-----------------------------------------------------------------------------

void GUI_ShortcutList::resized ()
{
	updateHeight ();
}
//-----------------------------------------------------------------------------

void GUI_ShortcutList::lookAndFeelChanged ()
{
	updateHeight ();
}
//-----------------------------------------------------------------------------

void GUI_ShortcutList::updateHeight ()
{
	const auto	rowH = UI::paddingDef ( UI::paddings::shortcuts_row ).top;
	const auto	headerH = UI::paddingDef ( UI::paddings::shortcuts_header ).top;

	auto	height = 0.0f;
	for ( const auto& item : items )
		height += item.header ? headerH : rowH;

	if ( const auto h = juce::roundToInt ( height ); h != getHeight () )
		setSize ( getWidth (), h );
}
//-----------------------------------------------------------------------------

void GUI_ShortcutList::paint ( juce::Graphics& g )
{
	const auto	rowH = UI::paddingDef ( UI::paddings::shortcuts_row ).top;
	const auto	headerH = UI::paddingDef ( UI::paddings::shortcuts_header ).top;
	const auto	clip = g.getClipBounds ().toFloat ();
	const auto	width = float ( getWidth () );
	const auto	style = CapStyle {};
	const auto	orText = " " + strings->get ( "shortcuts/or" ) + " ";

	auto	y = 0.0f;

	for ( const auto& item : items )
	{
		const auto	h = item.header ? headerH : rowH;
		const auto	row = juce::Rectangle<float> ( float ( leftInset ), y, width - float ( leftInset ), h );
		y += h;

		if ( row.getBottom () < clip.getY () || row.getY () > clip.getBottom () )
			continue;

		if ( item.header )
		{
			// Bottom-aligned: the extra height above is the gap to the previous section
			g.setFont ( UI::font ( UI::fonts::shortcuts_section ) );
			g.setColour ( findColour ( UI::colors::text ) );
			g.drawText ( strings->get ( item.text ), row.withTrimmedTop ( h - rowH ), juce::Justification::centredLeft, false );
		}
		else
		{
			g.setFont ( UI::font ( UI::fonts::dialog_entry ) );
			g.setColour ( findColour ( UI::colors::text ) );
			g.drawText ( strings->get ( item.text ), row, juce::Justification::centredLeft, false );

			// Right-aligned, an "or" between alternative chords
			auto	x = row.getRight ();

			for ( auto chord = item.chords->rbegin (); chord != item.chords->rend (); ++chord )
			{
				if ( chord != item.chords->rbegin () )
				{
					const auto	orFont = UI::font ( UI::fonts::shortcuts_hint );
					const auto	orW = juce::GlyphArrangement::getStringWidth ( orFont, orText );

					x -= orW;
					g.setFont ( orFont );
					g.setColour ( findColour ( UI::colors::textMuted ) );
					g.drawText ( orText, juce::Rectangle<float> ( x, row.getY (), orW, h ), juce::Justification::centred, false );
				}

				x -= chordWidth ( style, *chord );
				drawChord ( g, *this, style, *chord, x, row.getCentreY () );
			}
		}
	}
}
//-----------------------------------------------------------------------------

GUI_ShortcutHint::GUI_ShortcutHint ( const juce::String& verb )
{
	for ( const auto& entry : shortcuts->getEntries () )
		if ( entry.verb == verb )
			chords = &entry.chords;
}
//-----------------------------------------------------------------------------

// "Press {} to toggle": the caps of every chord replace the placeholder, an
// "or" between alternatives

void GUI_ShortcutHint::paint ( juce::Graphics& g )
{
	if ( ! chords )
		return;

	const auto	style = CapStyle {};
	const auto	font = UI::font ( UI::fonts::shortcuts_hint );
	const auto	text = strings->get ( "shortcuts/hint" );
	const auto	prefix = text.upToFirstOccurrenceOf ( "{}", false, false );
	const auto	suffix = text.fromFirstOccurrenceOf ( "{}", false, false );
	const auto	orText = " " + strings->get ( "shortcuts/or" ) + " ";
	const auto	centreY = getLocalBounds ().toFloat ().getCentreY ();
	const auto	textH = font.getHeight ();

	auto	drawWord = [ & ] ( const juce::String& word, float& x )
	{
		const auto	w = juce::GlyphArrangement::getStringWidth ( font, word );

		g.setFont ( font );
		g.setColour ( findColour ( UI::colors::text ) );
		g.drawText ( word, juce::Rectangle<float> ( x, centreY - textH * 0.5f, w, textH ), juce::Justification::centredLeft, false );

		x += w;
	};

	auto	x = 0.0f;

	drawWord ( prefix, x );

	for ( auto chord = chords->begin (); chord != chords->end (); ++chord )
	{
		if ( chord != chords->begin () )
			drawWord ( orText, x );

		x = drawChord ( g, *this, style, *chord, x, centreY );
	}

	drawWord ( suffix, x );
}
//-----------------------------------------------------------------------------

GUI_Shortcuts::GUI_Shortcuts ()
	: GUI_ModalPanel ( "shortcuts", "closeShortcuts" )
{
	title.setName ( "title" );
	hint.setName ( "hint" );
	viewport.setName ( "list" );

	viewport.setScrollBarsShown ( true, false );
	viewport.setViewedComponent ( &list, false );
	viewport.getProperties ().set ( "focusMargin", "2" );

	body.addAndMakeVisible ( title );
	body.addAndMakeVisible ( hint );
	body.addAndMakeVisible ( viewport );
}
//-----------------------------------------------------------------------------

void GUI_Shortcuts::resized ()
{
	GUI_ModalPanel::resized ();

	// The viewport is wider than the text column, the rows align with the title
	list.leftInset = title.getX () - viewport.getX ();
	list.setSize ( viewport.getMaximumVisibleWidth (), list.getHeight () );
	list.repaint ();
}
//-----------------------------------------------------------------------------
