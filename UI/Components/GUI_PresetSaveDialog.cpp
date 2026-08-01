#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_PresetSaveDialog.h"

#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_PresetSaveDialog::GUI_PresetSaveDialog ()
	: juce::Component ( "preset-save" )
{
	addAndMakeVisible ( body );

	title.setName ( "title" );
	body.addAndMakeVisible ( title );

	nameEditor.disableClickOnlyFocus ();
	nameEditor.onReturnPressed = [ this ] {	saveClicked ();	};
	nameEditor.onEscapePressed = [ this ] {	dismiss ();	};
	body.addAndMakeVisible ( nameEditor );

	save.onClick = [ this ] {	saveClicked ();	};
	body.addAndMakeVisible ( save );

	close.margin = 10.0f;
	close.bckAlpha[ 0 ] = 0.2f;
	close.bckAlpha[ 1 ] = 0.4f;
	close.bckMargin = 4.0f;
	close.setSize ( 32, 32 );
	close.setWantsKeyboardFocus ( false );
	close.onClick = [ this ] {	dismiss ();	};
	body.addAndMakeVisible ( close );

	setWantsKeyboardFocus ( true );
}
//-----------------------------------------------------------------------------

void GUI_PresetSaveDialog::paint ( juce::Graphics& g )
{
	// The veil darkens the settings behind it: this is modal
	g.fillAll ( juce::Colours::black.withAlpha ( 0.75f ) );

	// Built here, not cached: the owner's layout hot-reload moves the body
	// without telling us
	const auto	r = body.getBounds ().toFloat ();

	juce::Path	p;
	p.addRoundedRectangle ( r, UI::corner ( UI::corners::dialog_body, r ) );

	shadow.render ( g, p );

	g.setColour ( findColour ( juce::TooltipWindow::backgroundColourId ) );
	g.fillPath ( p );
}
//-----------------------------------------------------------------------------

bool GUI_PresetSaveDialog::keyPressed ( const juce::KeyPress& key )
{
	if ( key == juce::KeyPress ( juce::KeyPress::escapeKey, juce::ModifierKeys::noModifiers, 0 ) )
	{
		dismiss ();
		return true;
	}

	if ( key == juce::KeyPress ( juce::KeyPress::returnKey, juce::ModifierKeys::noModifiers, 0 ) )
	{
		saveClicked ();
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------

void GUI_PresetSaveDialog::visibilityChanged ()
{
	if ( ! isVisible () )
		return;

	// Resolved on every open, so theme edits are picked up without a restart.
	// Top indent stays 0: the default pushes the text below center
	nameEditor.setFont ( UI::font ( UI::fonts::dialog_entry ) );
	nameEditor.setIndents ( int ( UI::paddingDef ( UI::paddings::dialog_entry ).left ), 0 );
}
//-----------------------------------------------------------------------------

void GUI_PresetSaveDialog::show ( const juce::String& suggestedName )
{
	nameEditor.setText ( suggestedName, juce::dontSendNotification );

	setVisible ( true );
	toFront ( true );

	nameEditor.selectAllText ();

	outsideClick.start ();
}
//-----------------------------------------------------------------------------

void GUI_PresetSaveDialog::dismiss ()
{
	outsideClick.stop ();

	setVisible ( false );
}
//-----------------------------------------------------------------------------

void GUI_PresetSaveDialog::saveClicked ()
{
	const auto	name = nameEditor.getText ().trim ();
	if ( name.isEmpty () )
		return;

	if ( onSave )
		onSave ( name );
}
//-----------------------------------------------------------------------------
