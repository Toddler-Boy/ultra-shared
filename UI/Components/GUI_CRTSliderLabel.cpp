#include "ultra-shared/UI/UI_Helpers.h"
#include "GUI_CRTSliderLabel.h"

//-----------------------------------------------------------------------------

GUI_CRTSliderLabel::GUI_CRTSliderLabel ( const juce::String& labelKey, const bool bidirectional, const double maxValue, const double step )
	: label ( labelKey, UI::fonts::crt_label, UI::colors::text )
{
	label.setName ( "label" );
	slider.setName ( "slider" );

	slider.setScrollWheelEnabled ( false );
	slider.setRange ( bidirectional ? -maxValue : 0.0, maxValue, step );
	slider.setTextBoxStyle ( juce::Slider::TextBoxRight, true, 30, 20 );

	addAndMakeVisible ( label );
	addAndMakeVisible ( slider );
}
//-----------------------------------------------------------------------------

GUI_CRTSliderLabel::GUI_CRTSliderLabel ( const juce::String& setSection, const juce::String& setName, const bool bidirectional,
										 const double maxValue, const double step )
	: GUI_CRTSliderLabel ( "crt/settings/" + setSection + "/" + setName, bidirectional, maxValue, step )
{
	settingSection = setSection;
	settingName = setName;

	slider.setDoubleClickReturnValue ( true, preferences->getDefault<double> ( settingSection + "/" + settingName ) );
	slider.setValue ( currentValue (), juce::dontSendNotification );

	slider.onValueChange = [ this ]
	{
		preferences->set ( settingSection + "/" + settingName, slider.getValue () );

		if ( onValueChange )
			onValueChange ();
	};
}
//-----------------------------------------------------------------------------

GUI_CRTSliderLabel::GUI_CRTSliderLabel ( const juce::String& labelKey, float& value, const bool bidirectional,
										 const double maxValue, const double step )
	: GUI_CRTSliderLabel ( labelKey, bidirectional, maxValue, step )
{
	boundValue = &value;

	slider.setDoubleClickReturnValue ( true, value );
	slider.setValue ( currentValue (), juce::dontSendNotification );

	slider.onValueChange = [ this ]
	{
		*boundValue = float ( slider.getValue () );

		if ( onValueChange )
			onValueChange ();
	};
}
//-----------------------------------------------------------------------------

double GUI_CRTSliderLabel::currentValue () const
{
	return boundValue ? double ( *boundValue ) : preferences->get<double> ( settingSection + "/" + settingName );
}
//-----------------------------------------------------------------------------

void GUI_CRTSliderLabel::restorePreference ()
{
	slider.setValue ( currentValue (), juce::dontSendNotification );
}
//-----------------------------------------------------------------------------

void GUI_CRTSliderLabel::resized ()
{
	UI::setLayout ( layout, {	"UI/layouts/constants.json",
							"UI/layouts/components/crt-slider-label.json" } );
}
//-----------------------------------------------------------------------------
