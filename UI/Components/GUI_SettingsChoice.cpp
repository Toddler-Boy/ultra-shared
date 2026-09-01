#include "ultra-shared/UI/Components/GUI_SettingsChoice.h"

#include "Helpers/Messages.h"
#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SettingsChoice::GUI_SettingsChoice ( const juce::String& setSection, const juce::String& setName, const juce::StringArray& _options )
	: juce::Component ( setSection + "-" + setName )
	, settingSection ( setSection )
	, settingName ( setName )
	, options ( _options )
	, label ( "settings/" + setSection + "/" + setName, UI::fonts::settings_entry, UI::colors::text )
	, help ( "settings/" + setSection + "/" + setName + "-help", UI::fonts::settings_help, UI::colors::textMuted )
{
	help.setName ( "help" );

	// The options are frozen storage tokens; the combo shows the optional
	// "settings/<section>/<name>-<option>" text, falling back to the token
	for ( auto i = 0; i < options.size (); ++i )
		choice.addItem ( strings->getOptional ( "settings/" + setSection + "/" + setName + "-" + options[ i ].toLowerCase (), options[ i ] ), i + 1 );

	choice.setScrollWheelEnabled ( false );

	choice.onChange = [ this ]
	{
		const auto	id = choice.getSelectedId ();
		if ( id <= 0 )
			return;

		preferences->set ( settingSection + "/" + settingName, options[ id - 1 ] );

		msg::SettingChanged { settingSection, settingName }.send ();

		if ( onChanged )
			onChanged ();
	};

	addAndMakeVisible ( label );
	addAndMakeVisible ( help );
	addAndMakeVisible ( choice );
}
//-----------------------------------------------------------------------------

void GUI_SettingsChoice::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/settings-choice.json" } );
}
//-----------------------------------------------------------------------------

void GUI_SettingsChoice::restorePreference ()
{
	// An unknown stored value selects nothing, the consumers fall back for it
	choice.setSelectedId ( options.indexOf ( preferences->get<juce::String> ( settingSection + "/" + settingName ), true ) + 1, juce::dontSendNotification );
}
//-----------------------------------------------------------------------------
