#include "ultra-shared/UI/Components/GUI_SearchBar.h"

#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/UI/GUI_LookAndFeel.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SearchBar::GUI_SearchBar ()
{
	setName ( "searchBar" );

	// Mark this component as a searchBar
	getProperties ().set ( "type", "searchBar" );

	textEditor.disableClickOnlyFocus ();
	textEditor.addListener ( this );
	textEditor.onGetScreenBounds = [ this ]	{	return getScreenBounds ();	};
	lookAndFeelChanged ();

	textEditor.setColour ( juce::TextEditor::outlineColourId, juce::Colours::transparentBlack );
	textEditor.setColour ( juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack );

	addAndMakeVisible ( textEditor );

	clearSearch.margin = 4.0f;
	clearSearch.alpha[ 0 ] = 0.5f;
	clearSearch.useOrgSize = false;
	clearSearch.setWantsKeyboardFocus ( false );

	addChildComponent ( clearSearch );

	clearSearch.onClick = [ this ]	{
		textEditor.setText ( "" );
		textEditor.grabKeyboardFocus ();
	};
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::resized ()
{
	// Place text-editor and delete button
	{
		const auto	h = getHeight ();

		auto	b = getLocalBounds ().reduced ( 0, 2 );

		// Leave room on the left for the icon
		b.removeFromLeft ( h );

		// Place clear button at the end
		clearSearch.setBounds ( b.removeFromRight ( h ).reduced ( 0, 5 ) );

		// Place text-editor
		textEditor.setBounds ( b.withSizeKeepingCentre ( b.getWidth (), b.getHeight () - 4  ).translated ( 0, -2 ) );
	}
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::paint ( juce::Graphics& g )
{
	const auto	b = getLocalBounds ().toFloat ();
	const auto	radius = UI::corner ( UI::corners::search_bar, b );

	// Fill background
	{
		g.setColour ( textEditor.findColour ( juce::TextEditor::backgroundColourId ) );
		g.fillRoundedRectangle ( b, radius );
	}

	// Outline
	if ( textEditor.isEnabled () && ! textEditor.isReadOnly () && textEditor.hasKeyboardFocus ( true ) )
	{
		g.setColour ( findColour ( juce::TextEditor::focusedOutlineColourId ) );
		GUI_LookAndFeel::drawOutline ( g, b, radius, UI::lineWidth ( UI::lines::search_bar ) );
	}

	// Icon
	{
		const auto	col = textEditor.findColour ( juce::TextEditor::textColourId );
		const auto	rect = juce::Rectangle<float> ( radius * 0.33f, 0.0f, getHeight (), getHeight () );

		const juce::SharedResourcePointer<Icons>	icons;

		g.setColour ( col );
		g.fillPath ( UI::getScaledPath ( icons->get ( "search_bar/search" ), rect, 0, 0.3f ) );
	}
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::mouseDown ( const juce::MouseEvent& /*event*/ )
{
	textEditor.grabKeyboardFocus ();
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::focusGained ( juce::Component::FocusChangeType /*cause*/ )
{
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::focusLost ( juce::Component::FocusChangeType /*cause*/ )
{
	repaint ();
}
//-----------------------------------------------------------------------------

void GUI_SearchBar::lookAndFeelChanged ()
{
	textEditor.setFont ( UI::font ( UI::fonts::search_bar ) );
	textEditor.applyFontToAllText ( textEditor.getFont () );
}
//----------------------------------------------------------------------------------

void GUI_SearchBar::textEditorTextChanged ( juce::TextEditor& e )
{
	clearSearch.setVisible ( e.getText ().isNotEmpty () );
	clearSearch.repaint ();

	if ( onTextChange )
		onTextChange ();
}
//----------------------------------------------------------------------------------

void GUI_SearchBar::updateClearButton ()
{
	clearSearch.setVisible ( textEditor.getText ().isNotEmpty () );
}
//----------------------------------------------------------------------------------
