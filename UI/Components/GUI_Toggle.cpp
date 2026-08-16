#include "GUI_Toggle.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//----------------------------------------------------------------------------------

// A full 0-to-1 swing takes this long; a mid-flight reversal travels at the
// same speed over the shorter distance
constexpr auto	swingMs = 85.0;

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
	// Advance the animation by the wall time since it started
	const auto	target = getToggleState () ? 1.0f : 0.0f;

	if ( ! juce::approximatelyEqual ( animPosition, target ) )
	{
		const auto	travelled = float ( ( juce::Time::getMillisecondCounterHiRes () - animStartTime ) / swingMs );

		animPosition = animStartPosition < target ? std::min ( target, animStartPosition + travelled )
												  : std::max ( target, animStartPosition - travelled );
	}

	auto	b = getLocalBounds ().toFloat ();
	b.reduce ( 0.0f, b.getHeight () * 0.1f );

	//
	// Checkbox
	//
	{
		// Background
		{
			const auto	onCol = findColour ( UI::accent );
			const auto	offCol = UI::getShade ( 0.0f );

			g.setColour ( offCol.interpolatedWith ( onCol, animPosition * animPosition ) );
			g.fillRoundedRectangle ( b, b.getHeight () / 2.0f );
		}

		// Circle
		{
			const auto	r = b.reduced ( b.getHeight () * 0.1f );

			const auto	w = r.getHeight ();
			const auto	circleBounds = r.withWidth ( w ).translated ( ( r.getWidth () - w ) * animPosition, 0.0f ).reduced ( 1.5f );
			const auto	radius = circleBounds.getHeight () / 2.0f;

			g.setColour ( UI::getShade ( animPosition * 0.7f + 0.3f ) );
			g.fillRoundedRectangle ( circleBounds, radius );
		}
	}

	// Still moving: painting again next v-blank keeps the loop alive,
	// arriving simply stops asking
	if ( ! juce::approximatelyEqual ( animPosition, target ) )
		repaint ();
}
//----------------------------------------------------------------------------------

void GUI_Toggle::buttonStateChanged ()
{
	const auto	newState = getToggleState () ? 1.0f : 0.0f;

	if ( juce::approximatelyEqual ( newState, animPosition ) )
		return;

	// Not visible = nothing to animate, snap
	if ( ! isShowing () )
	{
		animPosition = newState;
		return;
	}

	// A mid-flight flip continues from where the knob is
	animStartPosition = animPosition;
	animStartTime = juce::Time::getMillisecondCounterHiRes ();

	repaint ();
}
//----------------------------------------------------------------------------------
