#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_SettingsAction.h"

#include "UI/ui-colors.h"
#include "ultra-shared/Helpers/MessageRouter.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_SettingsAction::GUI_SettingsAction ( const juce::String& name, const juce::String& verb )
	: juce::Component ( name )
	, label ( "settings/" + name + "/title", UI::fonts::settings_entry, UI::colors::text )
	, help ( "settings/" + name + "/help", UI::fonts::settings_help, UI::colors::textMuted )
	, button ( "button", "settings/" + name + "/button" )
{
	help.setName ( "help" );

	addAndMakeVisible ( label );
	addAndMakeVisible ( help );
	addAndMakeVisible ( button );

	button.onClick = [ verb ]
	{
		if ( auto* broadcaster = UI::ab.load () )
			broadcaster->sendActionMessage ( verb );
	};
}
//-----------------------------------------------------------------------------

void GUI_SettingsAction::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/settings-action.json" } );
}
//-----------------------------------------------------------------------------
