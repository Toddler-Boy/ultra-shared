#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_SolidButton.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

namespace
{
	[[ nodiscard ]] juce::Font labelFont ( const float height )
	{
		return UI::fontSized ( height * 0.45f, 600 );
	}
}
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
	g.fillRoundedRectangle ( b, cornerRole ? UI::corner ( *cornerRole, b ) : 6.0f );

	g.setColour ( labelColour.value_or ( findColour ( UI::colors::window ) ) );
	g.setFont ( labelFontOverride.value_or ( labelFont ( b.getHeight () ) ) );
	g.drawText ( label (), b, juce::Justification::centred );
}
//-----------------------------------------------------------------------------

int GUI_SolidButton::textWidth ( const float height ) const
{
	return int ( std::ceil ( juce::GlyphArrangement::getStringWidth ( labelFontOverride.value_or ( labelFont ( height ) ), label () ) ) );
}
//-----------------------------------------------------------------------------

juce::String GUI_SolidButton::label () const
{
	return strings->get ( textKey ).replace ( "{}", argument );
}
//-----------------------------------------------------------------------------
