#pragma once

#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_SolidButton.h"

//-----------------------------------------------------------------------------

// A settings row whose button sends a message-bus verb: title and help line
// left, the solid button right. Strings: settings/<name>/title, help, button

class GUI_SettingsAction final : public juce::Component
{
public:
	GUI_SettingsAction ( const juce::String& name, const juce::String& verb );

	// juce::Component
	void resized () override;

private:
	gin::LayoutSupport	layout { *this };

	GUI_DynamicLabel	label;
	GUI_DynamicLabel	help;
	GUI_SolidButton		button;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SettingsAction )
};
//-----------------------------------------------------------------------------
