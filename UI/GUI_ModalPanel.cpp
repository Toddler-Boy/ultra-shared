#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_ModalPanel.h"

#include "ultra-shared/Helpers/MessageRouter.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_ModalPanel::GUI_ModalPanel ( const juce::String& name, const char* _closeVerb )
	: juce::Component ( name )
	, closeVerb ( _closeVerb )
	, layoutFile ( "UI/layouts/" + name + ".json" )
{
	setOpaque ( true );

	addAndMakeVisible ( body );

	close.margin = 14.0f;
	close.bckAlpha[ 0 ] = 0.2f;
	close.bckAlpha[ 1 ] = 0.4f;
	close.bckMargin = 6.0f;
	close.setSize ( 48, 48 );
	close.setWantsKeyboardFocus ( false );
	close.setMouseClickGrabsKeyboardFocus ( false );
	body.addAndMakeVisible ( close );

	close.onClick = [ this ] {	requestClose ();	};

	setWantsKeyboardFocus ( true );
}
//-----------------------------------------------------------------------------

void GUI_ModalPanel::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
								layoutFile } );

	// Above whatever the subclass laid out in the corner
	close.toFront ( false );
}
//-----------------------------------------------------------------------------

void GUI_ModalPanel::paint ( juce::Graphics& g )
{
	const auto	corner = UI::corner ( UI::corners::dialog_body );

	// Built here: the layout hot-reload can move the body without a resized() call
	if ( bodyBounds != body.getBounds () )
	{
		bodyBounds = body.getBounds ();
		shadowPath.clear ();

		const auto	r = bodyBounds.toFloat ();

		shadowPath.addRoundedRectangle ( r, corner );
	}

	if ( background.isValid () )
	{
		g.drawImage ( background, getBounds ().toFloat () );
	}
	else
	{
		// Draw only the overlay around the body, not the body itself
		juce::RectangleList<int>	bckRect;
		bckRect.add ( getLocalBounds () );
		bckRect.subtract ( bodyBounds.reduced ( std::ceil ( corner ) ) );
		g.setColour ( UI::getShade ( 0.15f ) );
		g.fillRectList ( bckRect );
	}

	// Draw the drop shadow around the body
	shadow.render ( g, shadowPath );

	// Draw the body
	g.setColour ( findColour ( juce::TooltipWindow::backgroundColourId ) );
	g.fillPath ( shadowPath );
}
//-----------------------------------------------------------------------------

bool GUI_ModalPanel::keyPressed ( const juce::KeyPress& key )
{
	if ( key == juce::KeyPress ( juce::KeyPress::escapeKey, juce::ModifierKeys::noModifiers, 0 ) )
	{
		requestClose ();
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------

void GUI_ModalPanel::setBackground ( juce::Component* comp )
{
	setBackground ( comp ? comp->createComponentSnapshot ( comp->getLocalBounds () ) : juce::Image {} );
}
//-----------------------------------------------------------------------------

void GUI_ModalPanel::setBackground ( juce::Image snapshot )
{
	background = std::move ( snapshot );

	if ( ! background.isValid () )
		return;

	gin::applyStackBlur ( background, 50 );
	gin::applyBlend ( background, gin::BlendMode::Normal, UI::getShade ( 0.25f ).withAlpha ( 0.5f ) );
}
//-----------------------------------------------------------------------------

void GUI_ModalPanel::requestClose () const
{
	if ( auto* broadcaster = UI::ab.load () )
		broadcaster->sendActionMessage ( closeVerb );
}
//-----------------------------------------------------------------------------
