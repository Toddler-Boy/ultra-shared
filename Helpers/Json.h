#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Helpers/FileUtils.h"

//-----------------------------------------------------------------------------

namespace json
{
	// Trimmed items; anything but an array gives an empty list
	[[ nodiscard ]] inline juce::StringArray toStrings ( const juce::var& v )
	{
		juce::StringArray	list;

		if ( const auto* items = v.getArray () )
			for ( const auto& item : *items )
				list.add ( item.toString ().trim () );

		return list;
	}

	[[ nodiscard ]] inline juce::var toVar ( const juce::StringArray& list )
	{
		juce::Array<juce::var>	items;

		for ( const auto& s : list )
			items.add ( s );

		return items;
	}

	// Crash-safe: the old file stays until the new one is complete
	[[ nodiscard ]] inline bool write ( const juce::File& f, const juce::var& v )
	{
		return f.getParentDirectory ().createDirectory () && fileutils::replaceFile ( f, juce::JSON::toString ( v ) + "\n" );
	}
}
//-----------------------------------------------------------------------------
