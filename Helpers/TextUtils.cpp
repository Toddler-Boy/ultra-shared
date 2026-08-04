#include <JuceHeader.h>

#include <fmt/xchar.h>
#include <locale>
#include <string>

#include "ultra-shared/Helpers/TextUtils.h"

//-----------------------------------------------------------------------------

const std::locale& textutils::userLocale ()
{
	// std::locale ( "" ) is expensive to build and throws when the environment's locale
	// name does not resolve, so it happens once
	static const auto	loc = []
	{
		try				{	return std::locale ( "" );		}
		catch ( ... )	{	return std::locale::classic ();	}
	} ();

	return loc;
}
//-----------------------------------------------------------------------------

juce::StringArray textutils::getFilteredStrings ( const juce::StringArray& arr, const juce::StringArray& ext )
{
	juce::StringArray	filteredResults;

	for ( const auto& item : arr )
		for ( const auto& suffix : ext )
			if ( item.endsWithIgnoreCase ( suffix ) )
				filteredResults.add ( item );

	return filteredResults;
}
//-----------------------------------------------------------------------------

juce::String textutils::getHumanNumber ( const int64_t number )
{
	// Formatted wide: numpunct<char> can only hand back a single byte, so a locale that
	// groups with a no-break space would arrive as one invalid UTF-8 byte. juce::String
	// converts from wchar_t properly
	return juce::String ( fmt::format ( userLocale (), L"{:L}", number ).c_str () );
}
//-----------------------------------------------------------------------------
