#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Strings.h"

#include "ultra-shared/UI/Components/GUI_Label.h"

//-----------------------------------------------------------------------------

// A free-text preference (with description text), stored trimmed when the
// edit finishes
class GUI_SettingsText final : public juce::Component
{
public:
	GUI_SettingsText ( const juce::String& setSection, const juce::String& setName );

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

	GUI_DynamicLabel	label;
	GUI_DynamicLabel	help;
	juce::TextEditor	text { "text" };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SettingsText )
};
//-----------------------------------------------------------------------------
