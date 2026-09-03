#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_About.h"

#include "ultra-shared/Config/DataSource.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_About::GUI_About ()
	: GUI_ModalPanel ( "about", "closeAbout" )
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

	body.addAndMakeVisible ( icon );
	body.addAndMakeVisible ( title );
	body.addAndMakeVisible ( copyright );
	body.addAndMakeVisible ( link );

	scrollTextViewer.setName ( "scrollText" );
	scrollTextViewer.setFont ( UI::monoFont ( 12.0f, 400 ) );
	body.addAndMakeVisible ( scrollTextViewer );

	updateColors ();
	loadContent ();
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
