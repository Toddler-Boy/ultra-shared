#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/Resources/Theme.h"

//-----------------------------------------------------------------------------

// Settings drop-down listing every theme: the factory group first, then the
// user's own (marked with an icon) behind a separator. Items carry marked
// names ("$DATA$/default", "$USER$/neon"), the same form ui/theme stores.
// The list is rebuilt every time the popup opens, so file changes in either
// Themes folder never leave a stale menu
class GUI_ThemeSelector final : public juce::ComboBox
{
public:
	GUI_ThemeSelector ();

	// juce::ComboBox
	void showPopup () override;
	juce::MouseCursor getMouseCursor () override	{	return juce::MouseCursor::PointingHandCursor;	}

	// this: rebuilds the item list and re-selects the stored theme
	void restorePreference ();

private:
	// Item id - 1 indexes the marked theme name
	juce::StringArray	markedNames;

	juce::SharedResourcePointer<Theme>			theme;
	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Icons>			icons;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ThemeSelector )
};
//-----------------------------------------------------------------------------
