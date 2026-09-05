#include "GUI_FocusRing.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/GUI_LookAndFeel.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

constexpr auto	ringWidth = 2.0f;
constexpr auto	ringSlack = 1;		// anti-aliasing bleed around the stroke

// The area inside a ring that its stroke never touches
juce::Rectangle<int> GUI_FocusRing::holeOf ( const Ring& r )
{
	return r.rect.reduced ( int ( std::ceil ( std::max ( r.radius, ringWidth ) ) ) );
}
//-----------------------------------------------------------------------------

GUI_FocusRing::GUI_FocusRing ()
	: juce::Component ( "focusRing" )
{
	setInterceptsMouseClicks ( false, false );
	setWantsKeyboardFocus ( false );
	setAlwaysOnTop ( true );

	juce::Desktop::getInstance ().addGlobalMouseListener ( this );
}
//-----------------------------------------------------------------------------

GUI_FocusRing::~GUI_FocusRing ()
{
	juce::Desktop::getInstance ().removeGlobalMouseListener ( this );
}
//-----------------------------------------------------------------------------

void GUI_FocusRing::paint ( juce::Graphics& g )
{
	// Paints only when the dirty area touches the stroke: neither fully
	// outside the ring nor fully inside its hole
	if ( ring.rect.isEmpty () )
		return;

	const auto	clip = g.getClipBounds ();

	if ( ! clip.intersects ( ring.rect.expanded ( ringSlack ) ) || holeOf ( ring ).contains ( clip ) )
		return;

	g.setColour ( findColour ( UI::colors::accent ) );
	GUI_LookAndFeel::drawOutline ( g, ring.rect.toFloat (), ring.radius, ringWidth );
}
//-----------------------------------------------------------------------------

void GUI_FocusRing::mouseDown ( const juce::MouseEvent& /*e*/ )
{
	keyboardNav = false;

	watcher = nullptr;
	target = nullptr;
	update ();
}
//-----------------------------------------------------------------------------

bool GUI_FocusRing::hasOwnFocusVisual ( const juce::Component& c )
{
	return dynamic_cast<const juce::TextEditor*> ( &c ) != nullptr;
}
//-----------------------------------------------------------------------------

void GUI_FocusRing::focusChanged ( juce::Component* focused )
{
	watcher = nullptr;
	target = nullptr;

	// Keyboard navigation only, inside this window, never the parent or
	// components with a focus visual of their own
	if ( keyboardNav && focused
		 && focused != getParentComponent ()
		 && focused->getTopLevelComponent () == getTopLevelComponent ()
		 && ! hasOwnFocusVisual ( *focused ) )
	{
		target = focused;
		watcher = std::make_unique<Watcher> ( *focused, *this );
	}

	update ();
}
//-----------------------------------------------------------------------------

void GUI_FocusRing::update ()
{
	const auto	old = ring;
	ring = {};

	if ( target && target->isShowing () )
	{
		// The visible part only, clipped by every ancestor
		auto	area = getLocalArea ( target, target->getLocalBounds () );

		for ( auto p = target->getParentComponent (); p && p != getParentComponent (); p = p->getParentComponent () )
			area = area.getIntersection ( getLocalArea ( p, p->getLocalBounds () ) );

		if ( ! area.isEmpty () )
		{
			const auto&	props = target->getProperties ();

			// Margin: the theme role, scaled per side by the "focusMargin" property
			auto	margin = UI::paddingDef ( UI::paddings::focus_ring );

			if ( const auto& m = props[ "focusMargin" ]; m.isString () )
			{
				if ( const auto factors = UI::parsePadding ( m.toString () ) )
				{
					margin.top *= factors->top;
					margin.right *= factors->right;
					margin.bottom *= factors->bottom;
					margin.left *= factors->left;
				}
				else
				{
					Z_ERR ( "Bad focusMargin \"" << m.toString () << "\" on " << target->getName () );
				}
			}

			ring.rect = juce::BorderSize<int> ( juce::roundToInt ( margin.top ), juce::roundToInt ( margin.left ),
												juce::roundToInt ( margin.bottom ), juce::roundToInt ( margin.right ) ).addedTo ( area );

			// Radius: "focusRadius" as a number, or as the component's own corner
			// role (concentric: that corner plus the margin), else the theme role
			const auto	rectF = ring.rect.toFloat ();
			const auto	halfSide = std::min ( rectF.getWidth (), rectF.getHeight () ) / 2.0f;
			const auto&	r = props[ "focusRadius" ];

			if ( r.isDouble () || r.isInt () )
			{
				ring.radius = std::min ( float ( double ( r ) ), halfSide );
			}
			else if ( const auto role = r.isString () ? UI::corners::fromName ( r.toString () ) : UI::corners::count; role != UI::corners::count )
			{
				const auto	offset = ( margin.top + margin.right + margin.bottom + margin.left ) / 4.0f;

				ring.radius = std::min ( UI::corner ( role, area.toFloat () ) + std::max ( offset, 0.0f ), halfSide );
			}
			else
			{
				if ( r.isString () )
					Z_ERR ( "Unknown focusRadius role \"" << r.toString () << "\" on " << target->getName () );

				ring.radius = UI::corner ( UI::corners::focus_ring, rectF );
			}
		}
	}

	if ( ring == old )
		return;

	// Only the strokes: each ring's frame minus its own hole, then the union
	juce::RectangleList<int>	dirty;

	for ( const auto& r : { old, ring } )
	{
		if ( r.rect.isEmpty () )
			continue;

		juce::RectangleList<int>	stroke ( r.rect.expanded ( ringSlack ) );
		stroke.subtract ( holeOf ( r ) );

		dirty.add ( stroke );
	}

	for ( const auto& r : dirty )
		repaint ( r );
}
//-----------------------------------------------------------------------------
