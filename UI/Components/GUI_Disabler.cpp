#include "GUI_Disabler.h"

//-----------------------------------------------------------------------------

constexpr auto	disabledAlpha = 0.35f;

//-----------------------------------------------------------------------------

GUI_Disabler::GUI_Disabler ()
{
	setInterceptsMouseClicks ( false, true );
}
//-----------------------------------------------------------------------------

void GUI_Disabler::enablementChanged ()
{
	const auto	enabled = isEnabled ();

	setAlpha ( enabled ? 1.0f : disabledAlpha );
	setInterceptsMouseClicks ( false, enabled );
}
//-----------------------------------------------------------------------------
