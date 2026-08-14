#include "ultra-shared/UI/Components/GUI_SettingsText.h"

#include "Helpers/Messages.h"
#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SettingsText::GUI_SettingsText ( const juce::String& setSection, const juce::String& setName )
	: juce::Component ( setSection + "-" + setName )
	, settingSection ( setSection )
	, settingName ( setName )
	, label ( "settings/" + setSection + "/" + setName, UI::fonts::settings_entry, UI::colors::text )
	, help ( "settings/" + setSection + "/" + setName + "-help", UI::fonts::settings_help, UI::colors::textMuted )
{
	help.setName ( "help" );

	text.applyFontToAllText ( UI::font ( UI::fonts::settings_entry ), true );
	text.setIndents ( 4, 0 );
	text.setBorder ( {} );

	addAndMakeVisible ( label );
	addAndMakeVisible ( help );
	addAndMakeVisible ( text );

	auto finishedEdit = [ this ]
	{
		text.onFocusLost ();
		giveAwayKeyboardFocus ();
	};

	text.onReturnKey = finishedEdit;
	text.onEscapeKey = finishedEdit;

	text.onFocusLost = [ this ]
	{
		// Actual changes are announced, for settings that apply live
		const auto	key = settingSection + "/" + settingName;
		const auto	value = text.getText ().trim ();

		text.setText ( value, juce::dontSendNotification );

		if ( value != preferences->get<juce::String> ( key ) )
		{
			preferences->set ( key, value );
			msg::SettingChanged { settingSection }.send ();
		}
	};
}
//-----------------------------------------------------------------------------

void GUI_SettingsText::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/settings-text.json" } );

	lookAndFeelChanged ();
}
//-----------------------------------------------------------------------------

void GUI_SettingsText::lookAndFeelChanged ()
{
	const auto	txtCol = UI::getShade ( 1.0f );

	text.applyFontToAllText ( UI::font ( UI::fonts::settings_entry ), true );

	text.setColour ( juce::TextEditor::backgroundColourId, UI::getShade ( 0.2f ) );
	text.setColour ( juce::TextEditor::textColourId, txtCol );
	text.applyColourToAllText ( txtCol );
}
//-----------------------------------------------------------------------------

void GUI_SettingsText::restorePreference ()
{
	text.setText ( preferences->get<juce::String> ( settingSection + "/" + settingName ), juce::dontSendNotification );
}
//-----------------------------------------------------------------------------
