#pragma once

#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_Slider.h"

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

class GUI_CRTSliderLabel final : public juce::Component
{
public:
	GUI_CRTSliderLabel ( const juce::String& setSection, const juce::String& setName, const bool bidirectional = false,
						 const double maxValue = 100.0, const double step = 1.0 );

	// Bound to a plain value instead of a preference; its value at
	// construction serves as the double-click default
	GUI_CRTSliderLabel ( const juce::String& labelKey, float& value, const bool bidirectional = false,
						 const double maxValue = 100.0, const double step = 1.0 );

	// juce::Component
	void resized () override;

	// this: re-reads the preference (or bound value) into the slider (preset apply)
	void restorePreference ();

	std::function<void()>	onValueChange;

private:
	GUI_CRTSliderLabel ( const juce::String& labelKey, const bool bidirectional, const double maxValue, const double step );

	[[ nodiscard ]] double currentValue () const;

	gin::LayoutSupport	layout { *this };

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Strings>		strings;

	juce::String	settingSection;
	juce::String	settingName;

	float*	boundValue = nullptr;

	GUI_DynamicLabel	label;
	GUI_Slider			slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_CRTSliderLabel )
};
//-----------------------------------------------------------------------------
