#include "GUI_VIC2_Palette.h"

#include "ultra-shared/Video/colodore.h"

#include "Config/FilePaths.h"
#include "Helpers/Messages.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_VIC2_Palette::GUI_VIC2_Palette ()
{
	setOpaque ( true );

	setSettings ( 0, 50.0f, 100.0f, 50.0f, false );

	const auto	allowMouseClicks = filepaths::isDeveloperMode ();
	setInterceptsMouseClicks ( allowMouseClicks, allowMouseClicks );
}
//-----------------------------------------------------------------------------

void GUI_VIC2_Palette::paint ( juce::Graphics& g )
{
	auto	b = getLocalBounds ().toFloat ().expanded ( 0.0f, 1.0f );
	const auto	w = b.getWidth () / palette.size ();

	for ( auto index = 0; const auto& col : palette )
	{
		auto	r = b.removeFromLeft ( w ).withWidth ( w + 1.0f );

		if ( index++ == 0 )
			r.setLeft ( -1.0f );

		g.setColour ( col );
		g.fillRect ( r );
	}
}
//-----------------------------------------------------------------------------

void GUI_VIC2_Palette::setSettings ( const int standard, const float brightness, const float contrast, const float saturation, const bool earlyLuma )
{
	colodore	colo;

	const auto	yuvPal = colo.generateYUV ( standard, brightness, contrast, saturation, earlyLuma );
	const auto	rgbPal = colo.generateRGB ( standard, yuvPal );

	for ( auto index = 0; const auto col : rgbPal )
		palette[ index++ ] = juce::Colour ( col );

	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_VIC2_Palette::mouseDoubleClick ( const juce::MouseEvent& event )
{
	if ( event.mods.isAltDown () && event.mods.isCtrlDown () )
	{
		msg::ToggleFirstLumaAll {}.send ();
		return;
	}

	if ( event.mods.isAltDown () && event.mods.isShiftDown () )
	{
		msg::DeleteImage {}.send ();
		return;
	}

	if ( event.mods.isAltDown () )
	{
		msg::ToggleFirstLuma {}.send ();
		return;
	}

	if ( event.mods.isCtrlDown () )
	{
		msg::RemoveBorderColor {}.send ();
		return;
	}

	if ( event.mods.isShiftDown () )
	{
		msg::ToggleThumbnail {}.send ();
		return;
	}

	const auto	x = std::clamp ( event.x, 0, getWidth () - 1 ) / float ( getWidth () );
	const auto	index = int ( x * float ( palette.size () ) );

	msg::AssignBorderColor { index }.send ();
}
//-----------------------------------------------------------------------------
