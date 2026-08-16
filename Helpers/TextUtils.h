#pragma once

#include <JuceHeader.h>

#include <locale>

//-----------------------------------------------------------------------------

// std-only string helpers live in std_lime/lime_string_utils.h (lime::str::)

namespace textutils
{
	[[ nodiscard ]] juce::StringArray getFilteredStrings ( const juce::StringArray& arr, const juce::StringArray& ext );

	// True when text is an http(s) URL whose file name ends in one of exts
	[[ nodiscard ]] bool isUrlWithExtension ( const juce::String& text, const juce::StringArray& exts );

	// The environment's locale, resolved once; classic when the name does not resolve
	[[ nodiscard ]] const std::locale& userLocale ();

	// Grouped by the locale's own rules, so not always in threes (Indian: 12,34,567)
	[[ nodiscard ]] juce::String getHumanNumber ( const int64_t number );
}
//-----------------------------------------------------------------------------
