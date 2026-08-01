#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_About.h"

#include "Config/DataSource.h"
#include "Helpers/MessageRouter.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

static void broadcastClose ()
{
	if ( auto* broadcaster = UI::ab.load () )
		broadcaster->sendActionMessage ( "closeAbout" );
}
//-----------------------------------------------------------------------------

GUI_About::GUI_About ()
	: juce::Component ( "about" )
{
	icon.setName ( "icon" );
	title.setName ( "title" );
	copyright.setName ( "copyright" );

	// URL
	{
		const auto	url = "https://github.com/Toddler-Boy/" + juce::String ( ProjectInfo::projectName );

		link.setButtonText ( url );
		link.setURL ( juce::URL ( url ) );
		link.setName ( "link" );
		link.setFont ( UI::fontSized ( 20.0f, 700 ), false, juce::Justification::centredLeft );
	}

	{
		const auto	png = datasource::loadData ( "UI/png/about-icon.png" );
		icon.mipMap.setImage ( png.getData (), png.getSize () );
	}

	addAndMakeVisible ( body );

	body.addAndMakeVisible ( icon );
	body.addAndMakeVisible ( title );
	body.addAndMakeVisible ( copyright );
	body.addAndMakeVisible ( link );

	scrollTextViewer.setName ( "scrollText" );
	scrollTextViewer.setFont ( UI::monoFont ( 12.0f, 400 ) );
	body.addAndMakeVisible ( scrollTextViewer );

	updateColors ();

	closeAbout.margin = 14.0f;
	closeAbout.bckAlpha[ 0 ] = 0.2f;
	closeAbout.bckAlpha[ 1 ] = 0.4f;
	closeAbout.bckMargin = 6.0f;
	closeAbout.setSize ( 48, 48 );
	closeAbout.setWantsKeyboardFocus ( false );
	body.addAndMakeVisible ( closeAbout );

	closeAbout.onClick = [] {	broadcastClose ();	};

	setWantsKeyboardFocus ( true );

	loadContent ();
}
//-----------------------------------------------------------------------------

void GUI_About::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
								"UI/layouts/about.json" } );

	shadowPath.clear ();
	shadowPath.addRoundedRectangle ( body.getBounds ().toFloat (), 10.0f );
}
//-----------------------------------------------------------------------------

void GUI_About::paint ( juce::Graphics& g )
{
	shadow.render ( g, shadowPath );

	g.setColour ( findColour ( juce::TooltipWindow::backgroundColourId ) );
	g.fillPath ( shadowPath );
}
//-----------------------------------------------------------------------------

bool GUI_About::keyPressed ( const juce::KeyPress& key )
{
	if ( key == juce::KeyPress ( juce::KeyPress::escapeKey, juce::ModifierKeys::noModifiers, 0 ) )
	{
		broadcastClose ();
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------

void GUI_About::updateColors ()
{
	scrollTextViewer.setColour ( juce::Label::textColourId, UI::getShade ( 1.0f ) );
}
//-----------------------------------------------------------------------------

void GUI_About::loadContent ()
{
	// "{}" in the text becomes the compiled-in JUCE version, so the notice
	// never goes stale
	const auto	juceVersion = juce::String ( JUCE_MAJOR_VERSION ) + "." + juce::String ( JUCE_MINOR_VERSION ) + "." + juce::String ( JUCE_BUILDNUMBER );

	scrollTextViewer.setText ( datasource::loadText ( "UI/about.txt" ).replace ( "{}", juceVersion ) );
}
//-----------------------------------------------------------------------------
