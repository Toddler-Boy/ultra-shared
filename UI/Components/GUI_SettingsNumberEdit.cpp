#include <algorithm>
#include <cmath>

#include "ultra-shared/UI/Components/GUI_SettingsNumberEdit.h"

#include "Helpers/Messages.h"
#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

// "3" not "3.0", but "0.5" stays
[[ nodiscard ]] static juce::String formatFloat ( const float v )
{
	return juce::String ( v, 1 ).trimCharactersAtEnd ( "0" ).trimCharactersAtEnd ( "." );
}
//-----------------------------------------------------------------------------

GUI_SettingsNumberEdit::GUI_SettingsNumberEdit ( const juce::String& setSection, const juce::String& setName, const bool _isFloat )
	: juce::Component ( setSection + "-" + setName )
	, settingSection ( setSection )
	, settingName ( setName )
	, isFloat ( _isFloat )
	, label ( "settings/" + setSection + "/" + setName, UI::fonts::settings_entry, UI::colors::text )
	, help ( "settings/" + setSection + "/" + setName + "-help", UI::fonts::settings_help, UI::colors::textMuted )
{
	help.setName ( "help" );

	// Number field, digits only, no more of them than the maximum needs
	// (float fields get room and permission for one decimal place)
	const auto	range = Preferences::getRange ( setSection + "/" + setName );
	const auto	maxLength = juce::String ( range.max ).length () + ( isFloat ? 2 : 0 );
	number.setInputRestrictions ( maxLength, isFloat ? "0123456789." : "0123456789" );

	number.applyFontToAllText ( UI::font ( UI::fonts::settings_field ), true );
	number.setJustification ( juce::Justification::centred );
	number.setIndents ( 4, 0 );
	number.setBorder ( {} );

	addAndMakeVisible ( label );
	addAndMakeVisible ( help );
	addAndMakeVisible ( number );

	auto finishedEdit = [ this ]
	{
		number.onFocusLost ();
		giveAwayKeyboardFocus ();
	};

	number.onReturnKey = finishedEdit;
	number.onEscapeKey = finishedEdit;

	number.onFocusLost = [ this ]
	{
		// Clamp (quantized to 0.1 steps for float fields) and show what
		// actually stuck; an emptied field lands on the minimum. Actual
		// changes are announced, for settings that apply live (e.g. fx)
		const auto	key = settingSection + "/" + settingName;
		const auto	range = Preferences::getRange ( key );

		if ( isFloat )
		{
			const auto	value = std::round ( std::clamp ( number.getText ().getFloatValue (), float ( range.min ), float ( range.max ) ) * 10.0f ) / 10.0f;

			number.setText ( formatFloat ( value ), juce::dontSendNotification );

			if ( value != preferences->get<float> ( key ) )
			{
				preferences->set ( key, value );
				msg::SettingChanged { settingSection }.send ();
			}
		}
		else
		{
			const auto	value = std::clamp ( number.getText ().getIntValue (), range.min, range.max );

			number.setText ( juce::String ( value ), juce::dontSendNotification );

			if ( value != preferences->get<int> ( key ) )
			{
				preferences->set ( key, value );
				msg::SettingChanged { settingSection }.send ();
			}
		}
	};
}
//-----------------------------------------------------------------------------

void GUI_SettingsNumberEdit::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/settings-number.json" } );

	lookAndFeelChanged ();
}
//-----------------------------------------------------------------------------

void GUI_SettingsNumberEdit::lookAndFeelChanged ()
{
	const auto	txtCol = UI::getShade ( 1.0f );

	number.applyFontToAllText ( UI::font ( UI::fonts::settings_field ), true );

	number.setColour ( juce::TextEditor::backgroundColourId, UI::getShade ( 0.2f ) );
	number.setColour ( juce::TextEditor::textColourId, txtCol );
	number.applyColourToAllText ( txtCol );
}
//-----------------------------------------------------------------------------

void GUI_SettingsNumberEdit::restorePreference ()
{
	// Clamped, so out-of-range yml values display as what actually applies
	const auto	key = settingSection + "/" + settingName;

	if ( isFloat )
	{
		const auto	range = Preferences::getRange ( key );
		number.setText ( formatFloat ( std::clamp ( preferences->get<float> ( key ), float ( range.min ), float ( range.max ) ) ), juce::dontSendNotification );
	}
	else
		number.setText ( juce::String ( preferences->getClamped ( key ) ), juce::dontSendNotification );
}
//-----------------------------------------------------------------------------
