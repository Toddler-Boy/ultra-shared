#include "GUI_FocusRing.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/GUI_LookAndFeel.h"

//-----------------------------------------------------------------------------

constexpr auto	ringMargin = 3;
constexpr auto	ringRadius = 6.0f;
constexpr auto	ringWidth = 2.0f;

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
	if ( ring.isEmpty () )
		return;

	const auto	clip = g.getClipBounds ();

	if ( ! clip.intersects ( ring.expanded ( ringMargin ) ) || ring.reduced ( int ( std::ceil ( ringRadius ) ) ).contains ( clip ) )
		return;

	g.setColour ( findColour ( UI::colors::accent ) );
	GUI_LookAndFeel::drawOutline ( g, ring.toFloat (), ringRadius, ringWidth );
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
			ring = area.expanded ( ringMargin );
	}

	if ( ring == old )
		return;

	if ( ! old.isEmpty () )
		repaint ( old.expanded ( ringMargin ) );

	if ( ! ring.isEmpty () )
		repaint ( ring.expanded ( ringMargin ) );
}
//-----------------------------------------------------------------------------
