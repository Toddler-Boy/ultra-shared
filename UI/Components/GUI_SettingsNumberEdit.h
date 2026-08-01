#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Strings.h"

#include "ultra-shared/UI/Components/GUI_Label.h"

//-----------------------------------------------------------------------------

class GUI_SettingsNumberEdit final : public juce::Component
{
public:
	// isFloat allows one decimal place (0.1 steps); default is whole numbers
	GUI_SettingsNumberEdit ( const juce::String& setSection, const juce::String& setName, const bool isFloat = false );

	// juce::Component
	void resized () override;
	void lookAndFeelChanged () override;

	// this
	void restorePreference ();

private:
	gin::LayoutSupport	layout { *this };

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Strings>		strings;

	juce::String	settingSection;
	juce::String	settingName;
	bool			isFloat = false;

	GUI_DynamicLabel	label;
	GUI_DynamicLabel	help;
	juce::TextEditor	number { "number" };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SettingsNumberEdit )
};
//-----------------------------------------------------------------------------
