#include "GUI_SettingsBox.h"

#include "UI/GUI_LookAndFeel.h"

#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SettingsBox::GUI_SettingsBox ( const juce::String& n )
	: juce::Component ( n )
{
}
//-----------------------------------------------------------------------------

void GUI_SettingsBox::paint ( juce::Graphics& g )
{
	constexpr auto	blend = 0.067f;

	const auto	b = getLocalBounds ().toFloat ();

	g.setColour ( UI::getShade ( blend ) );
	GUI_LookAndFeel::drawOutlinedRect ( g, b, UI::corner ( UI::corners::settings_box, b ), UI::lineWidth ( UI::lines::settings_box ), UI::getShade ( blend * 2.0f ) );
}
//-----------------------------------------------------------------------------
