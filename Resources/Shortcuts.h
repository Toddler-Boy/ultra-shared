#pragma once

#include <JuceHeader.h>

#include <vector>

//-----------------------------------------------------------------------------

// The keyboard-shortcut table, loaded from UI/shortcuts.csv: columns
// section,verb,keys, row order = dialog order, keys = platform-neutral tokens
// joined by '+', '|' between alternative chords ("Ctrl+L|Ctrl+F"). One token
// map yields both the juce::KeyPress a chord matches and the keycaps the
// dialog draws, so the dialog can only ever show keys that work.

namespace shortcuts
{
	// One keycap: a text label, or a symbol svg (Data/UI/svg stem) for the
	// arrows, Enter and the mac modifier glyphs
	struct Cap
	{
		juce::String	text;
		juce::String	svg;
	};

	struct Chord
	{
		// Typed characters ("?") match on the text character, independent of
		// the keyboard layout; everything else on key code + modifiers
		juce::KeyPress		press;
		juce::juce_wchar	typed = 0;
		std::vector<Cap>	caps;

		[[ nodiscard ]] bool matches ( const juce::KeyPress& key ) const;
	};

	struct Entry
	{
		juce::String		section;
		juce::String		verb;
		std::vector<Chord>	chords;
	};

	// "Ctrl+Shift+Left" -> chord; an unknown token yields empty caps
	[[ nodiscard ]] Chord parseChord ( const juce::String& text );
}
//-----------------------------------------------------------------------------

class Shortcuts final
{
public:
	Shortcuts ();

	// The verb bound to a key, empty when unbound
	[[ nodiscard ]] juce::String find ( const juce::KeyPress& key ) const;

	[[ nodiscard ]] const std::vector<shortcuts::Entry>& getEntries () const	{	return entries;	}

private:
	std::vector<shortcuts::Entry>	entries;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( Shortcuts )
};
//-----------------------------------------------------------------------------
