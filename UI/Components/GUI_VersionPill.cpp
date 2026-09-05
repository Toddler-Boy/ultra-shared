#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_VersionPill.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

// Minimum spinner showtime, a sub-second check would look like a no-op
constexpr auto	minCheckingMS = 1000.0;

// No answer by then: drop the spinner, keep the previous state
constexpr auto	failsafeMS = 15000.0;

constexpr auto	elementGap = 4.0f;

//-----------------------------------------------------------------------------

GUI_VersionPill::GUI_VersionPill ()
	: juce::Button ( "version" )
{
	setMouseCursor ( juce::MouseCursor::PointingHandCursor );
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::setState ( const AppUpdater::State newState )
{
	// An install takes over from the checking spinner right away
	if ( newState == AppUpdater::State::updating )
	{
		if ( ! spinning () )
			spinStartMS = juce::Time::getMillisecondCounterHiRes ();

		checking = false;
		pending.reset ();
		progress = 0.0f;

		state = newState;
		fitToContent ();
		return;
	}

	// A visible spinner holds the result for its minimum showtime, a hidden one snaps
	if ( checking && isShowing () )
	{
		pending = newState;
		return;
	}

	if ( checking )
	{
		showResult ( newState );
		return;
	}

	state = newState;
	fitToContent ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::setProgress ( const float newProgress )
{
	progress = newProgress;
	fitToContent ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::clicked ()
{
	if ( spinning () )
		return;

	checking = true;
	pending.reset ();
	spinStartMS = juce::Time::getMillisecondCounterHiRes ();

	fitToContent ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::showResult ( const AppUpdater::State result )
{
	checking = false;
	pending.reset ();

	state = result;
	fitToContent ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::handleAsyncUpdate ()
{
	if ( ! checking )
		return;

	const auto	elapsed = juce::Time::getMillisecondCounterHiRes () - spinStartMS;

	if ( pending && elapsed >= minCheckingMS )
	{
		showResult ( *pending );
	}
	else if ( ! pending && elapsed >= failsafeMS )
	{
		checking = false;
		fitToContent ();
	}
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::fitToContent ()
{
	const auto	w = juce::roundToInt ( std::ceil ( pillWidth () ) );

	if ( w != getWidth () )
		setSize ( w, getHeight () );
	else
		repaint ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::resized ()
{
	fitToContent ();
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::lookAndFeelChanged ()
{
	fitToContent ();
}
//-----------------------------------------------------------------------------

bool GUI_VersionPill::spinning () const
{
	return checking || state == AppUpdater::State::updating;
}
//-----------------------------------------------------------------------------

juce::String GUI_VersionPill::currentText () const
{
	if ( checking )
		return strings->get ( "version-pill/checking" );

	if ( state == AppUpdater::State::updating )
		return juce::String ( int ( progress * 100.0f ) ) + "%";

	return ProjectInfo::versionString;
}
//-----------------------------------------------------------------------------

// Empty = no icon for this state
juce::String GUI_VersionPill::iconName () const
{
	if ( spinning () )
		return {};

	switch ( state )
	{
		case AppUpdater::State::current:
			return "version-pill/current";

		case AppUpdater::State::outdated:
			return "version-pill/outdated";

		case AppUpdater::State::downloadFailed:
		case AppUpdater::State::corrupted:
		case AppUpdater::State::replaceFailed:
			return "version-pill/failed";

		default:
			return {};
	}
}
//-----------------------------------------------------------------------------

float GUI_VersionPill::pillWidth () const
{
	const auto	h = float ( getHeight () );

	auto	w = h * 0.9f + juce::GlyphArrangement::getStringWidth ( UI::font ( UI::fonts::badge ), currentText () );

	if ( spinning () || iconName ().isNotEmpty () )
		w += elementGap + h * 0.6f;

	return w;
}
//-----------------------------------------------------------------------------

void GUI_VersionPill::paintButton ( juce::Graphics& g, bool isMouseOver, bool /*isMouseDown*/ )
{
	// The spinner's timing is detected here, applied outside the paint
	if ( checking )
	{
		const auto	elapsed = juce::Time::getMillisecondCounterHiRes () - spinStartMS;

		if ( ( pending && elapsed >= minCheckingMS ) || ( ! pending && elapsed >= failsafeMS ) )
			triggerAsyncUpdate ();
	}

	const auto	h = float ( getHeight () );
	const auto	b = getLocalBounds ().toFloat ();

	const auto	colId =	  spinning () ? UI::colors::statusUnknown
						: state == AppUpdater::State::current ? UI::colors::statusOk
						: state == AppUpdater::State::outdated ? UI::colors::statusWarning
						: state == AppUpdater::State::unknown ? UI::colors::statusUnknown
						: UI::colors::statusError;

	const auto	col = findColour ( colId );

	g.setColour ( col.withMultipliedAlpha ( isMouseOver ? 0.25f : 0.15f ) );
	g.fillRoundedRectangle ( b, UI::corner ( UI::corners::badge, b ) );

	g.setColour ( col );

	auto	content = b.reduced ( h * 0.45f, 0.0f );

	if ( const auto icon = iconName (); icon.isNotEmpty () )
	{
		g.fillPath ( UI::getScaledPath ( icons->get ( icon ), content.removeFromLeft ( h * 0.6f ), 0, 0.1f ) );
		content.removeFromLeft ( elementGap );
	}

	const auto	font = UI::font ( UI::fonts::badge );

	g.setFont ( font );
	g.drawText ( currentText (), content, juce::Justification::centredLeft, false );

	if ( spinning () )
	{
		const auto	d = h * 0.6f;
		const auto	spin = content.removeFromRight ( d ).withSizeKeepingCentre ( d, d ).reduced ( d * 0.1f );
		const auto	angle = float ( std::fmod ( juce::Time::getMillisecondCounterHiRes () - spinStartMS, 900.0 ) / 900.0 ) * juce::MathConstants<float>::twoPi;

		const juce::Graphics::ScopedSaveState	sss ( g );

		g.addTransform ( juce::AffineTransform::rotation ( angle, spin.getCentreX (), spin.getCentreY () ) );
		g.fillPath ( UI::getScaledPath ( icons->get ( "version-pill/checking" ), spin ) );

		// Keeps the spinner turning
		repaint ();
	}
}
//-----------------------------------------------------------------------------

juce::String GUI_VersionPill::getTooltip ()
{
	const auto	available = settings->get<juce::String> ( "update/last-known-version" );

	switch ( state )
	{
		case AppUpdater::State::current:
			return strings->get ( "version-pill/current" ).replace ( "{}", ProjectInfo::projectName );

		case AppUpdater::State::outdated:
			return strings->get ( AppUpdater::canInstall ? "version-pill/outdated" : "version-pill/outdated-check" ).replace ( "{}", available );

		case AppUpdater::State::updating:
			return strings->get ( "version-pill/updating" ).replace ( "{}", available );

		case AppUpdater::State::downloadFailed:
			return strings->get ( "version-pill/download-failed" );

		case AppUpdater::State::corrupted:
			return strings->get ( "version-pill/corrupted" );

		case AppUpdater::State::replaceFailed:
			return strings->get ( "version-pill/replace-failed" );

		default:
			return strings->get ( preferences->get<bool> ( "update/check" ) ? "version-pill/unknown" : "version-pill/disabled" );
	}
}
//-----------------------------------------------------------------------------
