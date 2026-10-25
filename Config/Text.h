#pragma once

#include <JuceHeader.h>

#include <initializer_list>
#include <utility>

#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

// A text of Data/UI/strings/en.yml with its {name} placeholders filled in, for code
// that has no Strings member of its own

namespace text
{
	[[ nodiscard ]] inline juce::String get ( const juce::String& key, std::initializer_list<std::pair<const char*, juce::String>> values = {} )
	{
		auto	out = juce::SharedResourcePointer<Strings> ()->get ( key );

		for ( const auto& [ name, value ] : values )
			out = out.replace ( "{" + juce::String ( name ) + "}", value );

		return out;
	}

	// For keys whose absence is normal (a display name per add-on): the fallback, silently
	[[ nodiscard ]] inline juce::String getOptional ( const juce::String& key, const juce::String& fallback )
	{
		return juce::SharedResourcePointer<Strings> ()->getOptional ( key, fallback );
	}
}
//-----------------------------------------------------------------------------
