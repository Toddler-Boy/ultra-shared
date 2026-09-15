#include "GUI_SVG_Button.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SVG_Button::GUI_SVG_Button ( const juce::String& buttonName, const juce::StringArray& _svgNames )
	: juce::Button ( buttonName )
{
	svgNames = _svgNames;

	GUI_SVG_Button::enablementChanged ();
}
//-----------------------------------------------------------------------------

void GUI_SVG_Button::clicked ( const juce::ModifierKeys& modifiers )
{
	juce::Button::clicked ( modifiers );

	stage = ( stage + 1 ) % svgNames.size ();
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_SVG_Button::paintButton ( juce::Graphics& g, bool isHover, bool /*isDown*/ )
{
	const auto	b = getLocalBounds ().toFloat ();
	const auto	alphaIndex = isHover ? 1 : 0;

	// Background
	{
		const auto	alpha = bckAlpha[ alphaIndex ];
		if ( alpha > 0.0f )
		{
			g.setColour ( findColour ( bckColId ).withMultipliedAlpha ( alpha ) );

			const auto	bckRect = b.reduced ( bckMargin );
			if ( bckRadius <= 0.01f )
				g.fillRect ( bckRect );
			else if ( bckRadius >= 0.49f * std::min ( bckRect.getWidth (), bckRect.getHeight () ) )
				g.fillEllipse ( bckRect );
			else
				g.fillRoundedRectangle ( bckRect, bckRadius );
		}
	}

	// Icon
	{
		const auto	icnAlpha = alpha[ alphaIndex ];

		g.setColour ( findColour ( colId ).withMultipliedAlpha ( icnAlpha ) );

		if ( useOrgSize )
		{
			const auto& p = UI::getScaledPathWithSize ( icons->get ( svgNames[ stage ] ), b.reduced ( margin ) + translation, placement );
			g.fillPath ( p );
		}
		else
		{
			const auto& p = UI::getScaledPath ( icons->get ( svgNames[ stage ] ), b.reduced ( margin ) + translation, placement );
			g.fillPath ( p );
		}
	}
}
//-----------------------------------------------------------------------------

void GUI_SVG_Button::enablementChanged ()
{
	juce::Button::enablementChanged ();

	setAlpha ( isEnabled () ? 1.0f : 0.5f );
	setMouseCursor ( isEnabled () ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::ParentCursor );
}
//-----------------------------------------------------------------------------

juce::String GUI_SVG_Button::getTooltip ()
{
	if ( const auto& str = juce::Button::getTooltip (); str.isNotEmpty () )
		return strings->get ( str + "_tip" );

	const auto&	ttStr = tooltips[ stage ];
	if ( ttStr.isEmpty () )
		return {};

	return strings->get ( ttStr + "_tip" );
}
//-----------------------------------------------------------------------------

int GUI_SVG_Button::getStage () const
{
	return stage;
}
//-----------------------------------------------------------------------------

void GUI_SVG_Button::setStage ( int newStage )
{
	stage = std::clamp ( newStage, 0, svgNames.size () - 1 );
	repaint ();
}
//-----------------------------------------------------------------------------

// The tooltip is the button's only text, so it names the button
std::unique_ptr<juce::AccessibilityHandler> GUI_SVG_Button::createAccessibilityHandler ()
{
	struct Handler final : juce::AccessibilityHandler
	{
		Handler ( GUI_SVG_Button& _button )
			: juce::AccessibilityHandler ( _button, juce::AccessibilityRole::button,
										   juce::AccessibilityActions ().addAction ( juce::AccessibilityActionType::press, [ &_button ] { _button.triggerClick (); } ) )
			, button ( _button )
		{
		}

		juce::String getTitle () const override	{	return button.getTooltip ();	}
		juce::String getHelp () const override	{	return button.getTooltip ();	}

		GUI_SVG_Button&	button;
	};

	return std::make_unique<Handler> ( *this );
}
//-----------------------------------------------------------------------------
