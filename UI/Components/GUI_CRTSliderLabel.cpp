#include "ultra-shared/UI/UI_Helpers.h"
#include "GUI_CRTSliderLabel.h"

//-----------------------------------------------------------------------------

GUI_CRTSliderLabel::GUI_CRTSliderLabel ( const juce::String& setSection, const juce::String& setName, const bool bidirectional,
										 const double maxValue, const double step )
	: settingSection ( setSection )
	, settingName ( setName )
	, label ( "crt/settings/" + setSection + "/" + setName, UI::fonts::crt_label, UI::colors::text )
{
	label.setName ( "label" );
	slider.setName ( "slider" );

	slider.setScrollWheelEnabled ( false );
	slider.setRange ( bidirectional ? -maxValue : 0.0, maxValue, step );
	slider.setTextBoxStyle ( juce::Slider::TextBoxRight, true, 30, 20 );

	slider.setDoubleClickReturnValue ( true, preferences->getDefault<double> ( settingSection + "/" + settingName ) );
	slider.setValue ( preferences->get<double> ( settingSection + "/" + settingName ), juce::dontSendNotification );

	slider.onValueChange = [ this ]
	{
		preferences->set ( settingSection + "/" + settingName, slider.getValue () );

		if ( onValueChange )
			onValueChange ();
	};

	addAndMakeVisible ( label );
	addAndMakeVisible ( slider );
}
//-----------------------------------------------------------------------------

void GUI_CRTSliderLabel::restorePreference ()
{
	slider.setValue ( preferences->get<double> ( settingSection + "/" + settingName ), juce::dontSendNotification );
}
//-----------------------------------------------------------------------------

void GUI_CRTSliderLabel::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/crt-slider-label.json" } );
}
//-----------------------------------------------------------------------------
