#include "ultra-shared/UI/Components/GUI_SettingsToggle.h"

#include "ultra-shared/UI/UI_Helpers.h"
#include "Helpers/Messages.h"
#include "UI/ui-colors.h"

//-----------------------------------------------------------------------------

GUI_SettingsToggle::GUI_SettingsToggle ( const juce::String& setSection, const juce::String& setName )
	: juce::Component ( setSection + "-" + setName )
	, settingSection ( setSection )
	, settingName ( setName )
	, label ( "settings/" + setSection + "/" + setName, UI::fonts::settings_entry, UI::colors::text)
	, help ( "settings/" + setSection + "/" + setName + "-help", UI::fonts::settings_help, UI::colors::textMuted )
	, toggle ( "toggle" )
{
	help.setName ( "help" );

	addAndMakeVisible ( label );
	addAndMakeVisible ( help );

	toggle.onClick = [ this ]
	{
		preferences->set ( settingSection + "/" + settingName, toggle.getToggleState () );

		msg::SettingChanged { settingSection, settingName }.send ();

		if ( onChanged )
			onChanged ();
	};

	addAndMakeVisible ( toggle );
}
//-----------------------------------------------------------------------------

void GUI_SettingsToggle::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/settings-toggle.json" } );
}
//-----------------------------------------------------------------------------

void GUI_SettingsToggle::restorePreference ()
{
	toggle.setToggleState ( preferences->get<bool> ( settingSection + "/" + settingName ), juce::dontSendNotification );
}
//-----------------------------------------------------------------------------
