#include "GUI_Toggle.h"

#include "ultra-shared/Helpers/Easings.h"
#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//----------------------------------------------------------------------------------

// A swing takes this long, whatever its distance. The knob's two edges ride
// offset s-curves: the tail's spans the whole swing, the lead's is compressed
// into its front by the stretch factor, the gap between them stretches the
// knob mid-flight
constexpr auto	swingMs = 160.0;
constexpr auto	stretch = 1.0f;

//----------------------------------------------------------------------------------

GUI_Toggle::GUI_Toggle ( const juce::String& name )
	: juce::ToggleButton ( name )
{
	enablementChanged ();
}
//----------------------------------------------------------------------------------

void GUI_Toggle::enablementChanged ()
{
	juce::ToggleButton::enablementChanged ();

	setAlpha ( isEnabled () ? 1.0f : 0.5f );
	setMouseCursor ( isEnabled () ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::ParentCursor );
}
//----------------------------------------------------------------------------------

void GUI_Toggle::paintButton ( juce::Graphics& g, bool /*isHover*/, bool /*isDown*/ )
{
	// Advance both knob edges by the wall time since the flip
	const auto	target = getToggleState () ? 1.0f : 0.0f;
	const auto	arrived = juce::approximatelyEqual ( animLead, target ) && juce::approximatelyEqual ( animTail, target );

	if ( ! arrived )
	{
		const auto	progress = float ( ( juce::Time::getMillisecondCounterHiRes () - animStartTime ) / swingMs );

		animLead = leadStart + ( target - leadStart ) * easing::smoothstep ( progress * ( 1.0f + stretch ) );
		animTail = tailStart + ( target - tailStart ) * easing::smoothstep ( progress );
	}

	auto	b = getLocalBounds ().toFloat ();
	b.reduce ( 0.0f, b.getHeight () * 0.1f );

	//
	// Checkbox
	//
	{
		const auto	mid = ( animLead + animTail ) * 0.5f;

		// Background
		{
			const auto	onCol = findColour ( UI::accent );
			const auto	offCol = UI::getShade ( 0.0f );

			g.setColour ( offCol.interpolatedWith ( onCol, mid * mid ) );
			g.fillRoundedRectangle ( b, b.getHeight () / 2.0f );
		}

		// Knob, spanning its two edges: a circle at rest, a pill while moving
		{
			const auto	r = b.reduced ( b.getHeight () * 0.1f );

			const auto	w = r.getHeight ();
			const auto	travel = r.getWidth () - w;
			const auto	lo = std::min ( animLead, animTail );
			const auto	hi = std::max ( animLead, animTail );

			const auto	knobBounds = r.withWidth ( w + travel * ( hi - lo ) ).translated ( travel * lo, 0.0f ).reduced ( 1.5f );

			g.setColour ( UI::getShade ( mid * 0.7f + 0.3f ) );
			g.fillRoundedRectangle ( knobBounds, knobBounds.getHeight () / 2.0f );
		}
	}

	// Still moving: painting again next v-blank keeps the loop alive,
	// arriving simply stops asking
	if ( ! arrived )
		repaint ();
}
//----------------------------------------------------------------------------------

void GUI_Toggle::buttonStateChanged ()
{
	const auto	newState = getToggleState () ? 1.0f : 0.0f;

	if ( juce::approximatelyEqual ( newState, animLead ) && juce::approximatelyEqual ( newState, animTail ) )
		return;

	// Not visible = nothing to animate, snap
	if ( ! isShowing () )
	{
		animLead = newState;
		animTail = newState;
		return;
	}

	// A mid-flight flip continues from where the knob edges are
	leadStart = animLead;
	tailStart = animTail;
	animStartTime = juce::Time::getMillisecondCounterHiRes ();

	repaint ();
}
//----------------------------------------------------------------------------------
