#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"

//-----------------------------------------------------------------------------

// A CRT preset is a yml fragment holding the tube-look subset of the crt
// preferences (noise through reflections); TV system, overlay and webcam
// settings stay personal, so one preset works under any standard or overlay.

namespace crtpresets
{
	// Marked preset names, the factory group first, each group sorted
	[[ nodiscard ]] juce::StringArray listPresets ();

	// Where a save with this name lands: the factory data of a developer
	// checkout, the user folder otherwise; invalid when neither is writable
	[[ nodiscard ]] juce::File saveTargetFile ( const juce::String& name );

	// Writes the live scope values to the save target; returns the marked
	// name, empty when the file cannot be written
	juce::String saveCurrentValues ( const Preferences& preferences, const juce::String& name );
}
//-----------------------------------------------------------------------------

class CRTPreset final
{
public:
	// An unknown or unreadable marked name leaves the preset invalid
	void load ( const juce::String& markedName );

	[[ nodiscard ]] bool isValid () const					{	return valid;	}
	[[ nodiscard ]] const juce::String& markedName () const	{	return marked;	}

	// Copies every value the preset defines into the preferences; a mask
	// bitmap that no longer exists is left untouched
	void applyTo ( Preferences& preferences ) const;

	// True when the live preferences equal every value the preset defines
	[[ nodiscard ]] bool matches ( const Preferences& preferences ) const;

private:
	struct entry
	{
		juce::String		path;	// "crt/<key>"
		YamlFile::ConfigValue	value;
	};

	// applyTo and matches share one walk: compareOnly turns every set into an
	// equality check and reports the first mismatch
	bool walk ( Preferences& preferences, bool compareOnly ) const;

	template <typename T>
	static bool applyOrMatch ( Preferences& preferences, const juce::String& path, const T& value, const bool compareOnly )
	{
		if ( compareOnly )
			return preferences.get<T> ( path ) == value;

		preferences.set ( path, value );
		return true;
	}

	bool				valid = false;
	juce::String		marked;
	std::vector<entry>	entries;
};
//-----------------------------------------------------------------------------
