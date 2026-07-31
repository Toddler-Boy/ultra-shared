#include "GUI_ThemeSelector.h"

#include "Config/FilePaths.h"
#include "Helpers/Messages.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_ThemeSelector::GUI_ThemeSelector ()
	: juce::ComboBox ( "theme" )
{
	setScrollWheelEnabled ( false );

	restorePreference ();

	onChange = [ this ]
	{
		const auto	id = getSelectedId ();
		if ( id <= 0 )
			return;

		preferences->set ( "ui/theme", markedNames[ id - 1 ] );

		msg::SettingChanged { "ui", "theme" }.send ();
	};
}
//-----------------------------------------------------------------------------

void GUI_ThemeSelector::showPopup ()
{
	restorePreference ();

	juce::ComboBox::showPopup ();
}
//-----------------------------------------------------------------------------

void GUI_ThemeSelector::restorePreference ()
{
	clear ( juce::dontSendNotification );
	markedNames.clear ();

	const auto	userMarker = filepaths::markerFor ( filepaths::root::user ) + "/";

	auto&	menu = *getRootMenu ();
	auto	factorySeen = false;

	for ( const auto& marked : theme->listThemes () )
	{
		const auto	isUser = marked.startsWith ( userMarker );

		// One separator between the factory and the user group (the list is
		// ordered factory-then-user)
		if ( isUser && factorySeen )
		{
			menu.addSeparator ();
			factorySeen = false;
		}
		else if ( ! isUser )
		{
			factorySeen = true;
		}

		markedNames.add ( marked );

		juce::PopupMenu::Item	item ( marked.fromFirstOccurrenceOf ( "/", false, false ) );
		item.itemID = markedNames.size ();
		item.setImage ( UI::getMenuIcon ( icons->get ( isUser ? "theme-user" : "theme-factory" ) ) );

		menu.addItem ( item );
	}

	// An unknown stored name selects nothing; Theme::load equally falls back
	// to the code defaults for it
	setSelectedId ( markedNames.indexOf ( preferences->get<juce::String> ( "ui/theme" ) ) + 1, juce::dontSendNotification );
}
//-----------------------------------------------------------------------------
