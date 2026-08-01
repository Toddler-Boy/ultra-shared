#include "ultra-shared/UI/Components/GUI_DesktopDropshadow.h"

#include "ultra-shared/UI/UI_Helpers.h"

//-------------------------------------------------------------------------------------------------

GUI_DesktopDropshadow::GUI_DesktopDropshadow ( juce::Component& o )
	: owner ( o )
{
	setInterceptsMouseClicks ( false, false );

	if ( auto p = o.getParentComponent () )
	{
		p->addAndMakeVisible ( this );
	}
	else
	{
		addToDesktop ( juce::ComponentPeer::windowIsTemporary |
					  juce::ComponentPeer::windowIgnoresKeyPresses |
					  juce::ComponentPeer::windowIgnoresMouseClicks );
		setAlwaysOnTop (true);
		setTransform ( juce::AffineTransform ().scaled ( owner.getDesktopScaleFactor () ) );
	}

	owner.addComponentListener ( this );
	updatePosition();
	toBehind ( &owner );
	setVisible ( true );
}
//-------------------------------------------------------------------------------------------------

GUI_DesktopDropshadow::~GUI_DesktopDropshadow ()
{
	owner.removeComponentListener ( this );
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::componentMovedOrResized ( juce::Component&, bool, bool )
{
	updatePosition ();
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::updatePosition ()
{
	setBounds ( owner.getBounds ().expanded ( 11 ) );

	shadowPath.clear ();

	// Only popup menus get one of these (preparePopupMenuWindow), so the
	// shadow follows the menu body's rounding
	const auto	r = getLocalBounds ().toFloat ().reduced ( 11.0f );
	shadowPath.addRoundedRectangle ( r, UI::corner ( UI::corners::menu_body, r ) );
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::componentBroughtToFront (juce::Component& )
{
	toBehind ( &owner );
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::componentVisibilityChanged ( juce::Component& )
{
	setVisible ( owner.isVisible () );
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::componentBeingDeleted ( juce::Component& )
{
	delete this;
}
//-------------------------------------------------------------------------------------------------

void GUI_DesktopDropshadow::paint ( juce::Graphics& g )
{
	shadow.render ( g, shadowPath );
}
//-------------------------------------------------------------------------------------------------
