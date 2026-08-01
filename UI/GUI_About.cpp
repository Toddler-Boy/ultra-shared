#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_About.h"

#include "Config/DataSource.h"
#include "Helpers/MessageRouter.h"
#include "ultra-shared/UI/UI_Helpers.h"

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

	about.addAndMakeVisible ( icon );
	about.addAndMakeVisible ( title );
	about.addAndMakeVisible ( copyright );
	about.addAndMakeVisible ( link );
	addAndMakeVisible ( about );

	scrollTextViewer.setName ( "scrollText" );
	scrollTextViewer.setFont ( UI::monoFont ( 16.0f, 400 ) );
	addAndMakeVisible ( scrollTextViewer );

	updateColors ();

	closeAbout.margin = 14.0f;
	closeAbout.bckAlpha[ 0 ] = 0.2f;
	closeAbout.bckAlpha[ 1 ] = 0.4f;
	closeAbout.bckMargin = 6.0f;
	closeAbout.setSize ( 48, 48 );
	closeAbout.setWantsKeyboardFocus ( false );
	addAndMakeVisible ( closeAbout );

	closeAbout.onClick = []
	{
		if ( auto* broadcaster = UI::ab.load () )
			broadcaster->sendActionMessage ( "closeAbout" );
	};

	loadContent ();
}
//-----------------------------------------------------------------------------

void GUI_About::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
								"UI/layouts/about.json" } );
}
//-----------------------------------------------------------------------------

void GUI_About::updateColors ()
{
	scrollTextViewer.setColour ( juce::Label::textColourId, UI::getShade ( 1.0f ) );
}
//-----------------------------------------------------------------------------

void GUI_About::loadContent ()
{
	scrollTextViewer.setText ( datasource::loadText ( "UI/about.txt" ) );
}
//-----------------------------------------------------------------------------
