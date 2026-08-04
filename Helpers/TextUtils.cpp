#include <JuceHeader.h>

#include <fmt/xchar.h>
#include <locale>
#include <string>

#include "ultra-shared/Helpers/TextUtils.h"
#include "ultra-shared/Helpers/PlatformHelper.h"

//-----------------------------------------------------------------------------

namespace
{
	// Grouping facet for machines where the environment lookup lands on "C"
	// (GUI apps on macOS get no LANG): the OS-configured grouping, and the
	// struct's defaults are the American fallback
	struct GroupedPunct final : std::numpunct<wchar_t>
	{
		explicit GroupedPunct ( const NumberGrouping& setGrouping ) : grouping ( setGrouping ) {}

		wchar_t do_thousands_sep () const override { return grouping.separator; }

		std::string do_grouping () const override
		{
			if ( grouping.groupSize <= 0 )
				return {};

			std::string	sizes ( 1, char ( grouping.groupSize ) );
			if ( grouping.secondaryGroupSize > 0 )
				sizes += char ( grouping.secondaryGroupSize );

			return sizes;
		}

		NumberGrouping	grouping;
	};
}
//-----------------------------------------------------------------------------

const std::locale& textutils::userLocale ()
{
	// std::locale ( "" ) is expensive to build and throws when the environment's locale
	// name does not resolve, so it happens once
	static const auto	loc = []
	{
		try
		{
			if ( auto l = std::locale ( "" ); l.name () != "C" && l.name () != "POSIX" )
				return l;
		}
		catch ( ... ) {}

		return std::locale ( std::locale::classic (), new GroupedPunct ( userNumberGrouping () ) );
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
