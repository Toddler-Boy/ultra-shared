#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

//-----------------------------------------------------------------------------

// boost::regex behind a compile-once handle. Invalid expressions and
// pathological inputs report no match instead of throwing

namespace regex
{
	class Pattern final
	{
	public:
		explicit Pattern ( const std::string& expression, bool caseInsensitive = false );
		~Pattern ();

		Pattern ( Pattern&& ) noexcept;
		Pattern& operator= ( Pattern&& ) noexcept;

		[[ nodiscard ]] bool isValid () const;

		// Anywhere in the text / the whole text
		[[ nodiscard ]] bool contains ( std::string_view text ) const;
		[[ nodiscard ]] bool matches ( std::string_view text ) const;

		// Capture groups of the first match, [0] = the whole match; empty = no match
		[[ nodiscard ]] std::vector<std::string> capture ( std::string_view text ) const;

		// Every match replaced with the literal replacement (no $1 expansion)
		[[ nodiscard ]] std::string replaceAll ( std::string_view text, std::string_view replacement ) const;

		// Every match replaced by the callback's return, groups[0] = the whole match
		[[ nodiscard ]] std::string replaceAll ( std::string_view text,
												 const std::function<std::string ( const std::vector<std::string>& groups )>& replace ) const;

		// The text split at the matches, in order; matched runs are flagged,
		// non-match segments are only emitted when non-empty
		struct Segment
		{
			std::string	text;
			bool		isMatch = false;
		};
		[[ nodiscard ]] std::vector<Segment> segments ( std::string_view text ) const;

	private:
		struct Impl;
		std::unique_ptr<Impl>	impl;
	};
}
//-----------------------------------------------------------------------------
