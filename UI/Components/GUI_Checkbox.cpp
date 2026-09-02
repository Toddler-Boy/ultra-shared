#include "GUI_Checkbox.h"

#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//----------------------------------------------------------------------------------

constexpr auto	cornerFraction = 0.2f;	// Of the box size
constexpr auto	markStroke = 2.0f;

//----------------------------------------------------------------------------------

GUI_Checkbox::GUI_Checkbox ( const juce::String& name )
	: juce::ToggleButton ( name )
{
	enablementChanged ();
}
//----------------------------------------------------------------------------------

void GUI_Checkbox::enablementChanged ()
{
	juce::ToggleButton::enablementChanged ();

	setAlpha ( isEnabled () ? 1.0f : 0.5f );
	setMouseCursor ( isEnabled () ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::ParentCursor );
}
//----------------------------------------------------------------------------------

void GUI_Checkbox::paintButton ( juce::Graphics& g, bool isHover, bool /*isDown*/ )
{
	// A square on the left, vertically centered
	const auto	size = float ( std::min ( getWidth (), getHeight () ) );
	const auto	b = juce::Rectangle<float> ( 0.0f, ( float ( getHeight () ) - size ) / 2.0f, size, size ).reduced ( 1.0f );
	const auto	radius = size * cornerFraction;
	const auto	on = getToggleState ();

	g.setColour ( on ? findColour ( UI::accent ) : UI::getShade ( isHover ? 0.1f : 0.0f ) );
	g.fillRoundedRectangle ( b, radius );

	if ( ! on )
	{
		g.setColour ( UI::getShade ( isHover ? 0.5f : 0.3f ) );
		g.drawRoundedRectangle ( b, radius, 1.0f );
		return;
	}

	// Check mark: short stroke down-right, long stroke up-right
	const auto	r = b.reduced ( size * 0.25f );

	juce::Path	mark;
	mark.startNewSubPath ( r.getX (), r.getY () + r.getHeight () * 0.55f );
	mark.lineTo ( r.getX () + r.getWidth () * 0.38f, r.getBottom () );
	mark.lineTo ( r.getRight (), r.getY () );

	g.setColour ( UI::getShade ( 1.0f ) );
	g.strokePath ( mark, juce::PathStrokeType ( markStroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded ) );
}
//----------------------------------------------------------------------------------
