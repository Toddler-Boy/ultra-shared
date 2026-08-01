#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_SolidButton.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SolidButton::GUI_SolidButton ( const juce::String& buttonName, const juce::String& stringsKey )
	: juce::Button ( buttonName )
	, textKey ( stringsKey )
{
}
//-----------------------------------------------------------------------------

void GUI_SolidButton::paintButton ( juce::Graphics& g, bool isHover, bool isDown )
{
	const auto	b = getLocalBounds ().toFloat ();

	auto	bckCol = findColour ( UI::colors::text );

	if ( isDown )
		bckCol = bckCol.withMultipliedAlpha ( 0.75f );
	else if ( isHover )
		bckCol = bckCol.withMultipliedAlpha ( 0.9f );

	g.setColour ( bckCol );
	g.fillRoundedRectangle ( b, 6.0f );

	g.setColour ( findColour ( UI::colors::window ) );
	g.setFont ( UI::fontSized ( b.getHeight () * 0.45f, 600 ) );
	g.drawText ( strings->get ( textKey ), b, juce::Justification::centred );
}
//-----------------------------------------------------------------------------
