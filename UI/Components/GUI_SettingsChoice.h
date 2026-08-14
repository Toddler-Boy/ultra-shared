#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Strings.h"

#include "ultra-shared/UI/Components/GUI_ComboBox.h"
#include "ultra-shared/UI/Components/GUI_Label.h"

//-----------------------------------------------------------------------------

// A fixed-vocabulary preference (with description text): the combo lists the
// given options and stores the picked one verbatim
class GUI_SettingsChoice final : public juce::Component
{
public:
	GUI_SettingsChoice ( const juce::String& setSection, const juce::String& setName, const juce::StringArray& options );

	// juce::Component
	void resized () override;

	// this
	void restorePreference ();

	// Fires after a pick has been stored, for siblings that display it
	std::function<void ()>	onChanged;

private:
	gin::LayoutSupport	layout { *this };

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Strings>		strings;

	juce::String		settingSection;
	juce::String		settingName;
	juce::StringArray	options;

	GUI_DynamicLabel	label;
	GUI_DynamicLabel	help;
	GUI_ComboBox		choice { "choice" };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SettingsChoice )
};
//-----------------------------------------------------------------------------
