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

	// juce::Component
	void resized () override;

	// this: re-reads the preference into the slider (preset apply)
	void restorePreference ();

	std::function<void()>	onValueChange;

private:
	gin::LayoutSupport	layout { *this };

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Strings>		strings;

	juce::String	settingSection;
	juce::String	settingName;

	GUI_DynamicLabel	label;
	GUI_Slider			slider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_CRTSliderLabel )
};
//-----------------------------------------------------------------------------
