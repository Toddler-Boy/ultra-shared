#pragma once

#include <JuceHeader.h>

#include <vector>

#include "ultra-shared/Resources/Shortcuts.h"
#include "ultra-shared/Resources/Strings.h"
#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_ViewportSmoothScroll.h"
#include "ultra-shared/UI/GUI_ModalPanel.h"

//-----------------------------------------------------------------------------

// The scrolled content of the shortcuts dialog: a section header per csv
// section, then one row per verb with its label left and its keycaps right.
// Painted in one pass, culled to the clip, heights come from the theme

class GUI_ShortcutList final : public juce::Component
{
public:
	GUI_ShortcutList ();

	// juce::Component
	void resized () override;
	void lookAndFeelChanged () override;
	void paint ( juce::Graphics& g ) override;

private:
	struct Item
	{
		bool								header;
		juce::String						text;		// the strings key
		const std::vector<shortcuts::Chord>*	chords;	// null for headers
	};

	void updateHeight ();

	juce::SharedResourcePointer<Shortcuts>	shortcuts;
	juce::SharedResourcePointer<Strings>	strings;

	std::vector<Item>	items;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ShortcutList )
};
//-----------------------------------------------------------------------------

// The "Press ? or Ctrl+/ to toggle" line: the hint string's "{}" becomes the
// keycaps of the given verb's chords, so the hint always names working keys

class GUI_ShortcutHint final : public juce::Component
{
public:
	explicit GUI_ShortcutHint ( const juce::String& verb );

	// juce::Component
	void paint ( juce::Graphics& g ) override;

private:
	juce::SharedResourcePointer<Shortcuts>	shortcuts;
	juce::SharedResourcePointer<Strings>	strings;

	const std::vector<shortcuts::Chord>*	chords = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ShortcutHint )
};
//-----------------------------------------------------------------------------

// The keyboard-shortcuts dialog both apps share (UI/layouts/shortcuts.json
// sizes the panel). Shown/hidden by the host via "showShortcuts"/
// "closeShortcuts"

class GUI_Shortcuts final : public GUI_ModalPanel
{
public:
	GUI_Shortcuts ();

	// juce::Component
	void resized () override;

private:
	GUI_DynamicLabel	title { "shortcuts/title", UI::fonts::shortcuts_title };
	GUI_ShortcutHint	hint { "showShortcuts" };

	juce::Viewport				viewport;
	GUI_ShortcutList			list;
	GUI_ViewportSmoothScroll	smoothScroll { viewport };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Shortcuts )
};
//-----------------------------------------------------------------------------
