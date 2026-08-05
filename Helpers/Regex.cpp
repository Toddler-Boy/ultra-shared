#include "Regex.h"

#include <boost/regex.hpp>

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

struct regex::Pattern::Impl
{
	boost::regex	re;
};
//-----------------------------------------------------------------------------

regex::Pattern::Pattern ( const std::string& expression, const bool caseInsensitive )
	: impl ( std::make_unique<Impl> () )
{
	auto	flags = boost::regex_constants::normal | boost::regex_constants::no_except;

	if ( caseInsensitive )
		flags |= boost::regex_constants::icase;

	impl->re.assign ( expression, flags );

	if ( ! isValid () )
		Z_ERR ( "Invalid regex: " << expression );
}
//-----------------------------------------------------------------------------

regex::Pattern::~Pattern () = default;
regex::Pattern::Pattern ( Pattern&& ) noexcept = default;
regex::Pattern& regex::Pattern::operator= ( Pattern&& ) noexcept = default;
//-----------------------------------------------------------------------------

bool regex::Pattern::isValid () const
{
	return impl->re.status () == 0;
}
//-----------------------------------------------------------------------------

// The catch blocks turn boost's runaway-input protection (complexity/stack
// limits) into a no-match; exceptions never leave this file

bool regex::Pattern::contains ( const std::string_view text ) const
{
	if ( ! isValid () )
		return false;

	const auto	begin = text.data () ? text.data () : "";

	try
	{
		return boost::regex_search ( begin, begin + text.size (), impl->re );
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up searching " << text.size () << " chars" );
		return false;
	}
}
//-----------------------------------------------------------------------------

bool regex::Pattern::matches ( const std::string_view text ) const
{
	if ( ! isValid () )
		return false;

	const auto	begin = text.data () ? text.data () : "";

	try
	{
		return boost::regex_match ( begin, begin + text.size (), impl->re );
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up matching " << text.size () << " chars" );
		return false;
	}
}
//-----------------------------------------------------------------------------

std::string regex::Pattern::replaceAll ( const std::string_view text, const std::string_view replacement ) const
{
	if ( ! isValid () )
		return std::string ( text );

	const auto	begin = text.data () ? text.data () : "";

	try
	{
		std::string	out;
		out.reserve ( text.size () );

		boost::regex_replace ( std::back_inserter ( out ), begin, begin + text.size (),
							   impl->re, std::string ( replacement ), boost::regex_constants::format_literal );

		return out;
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up replacing in " << text.size () << " chars" );
		return std::string ( text );
	}
}
//-----------------------------------------------------------------------------

std::string regex::Pattern::replaceAll ( const std::string_view text,
										 const std::function<std::string ( const std::vector<std::string>& groups )>& replace ) const
{
	if ( ! isValid () )
		return std::string ( text );

	const auto	begin = text.data () ? text.data () : "";
	const auto	end = begin + text.size ();

	try
	{
		std::string	out;
		out.reserve ( text.size () );

		auto	pos = begin;
		boost::match_results<const char*>	m;

		while ( pos != end && boost::regex_search ( pos, end, m, impl->re ) )
		{
			// An empty match would never advance; the remainder stays as is
			if ( m[ 0 ].first == m[ 0 ].second )
				break;

			out.append ( pos, m[ 0 ].first );

			std::vector<std::string>	groups;
			groups.reserve ( m.size () );

			for ( const auto& sub : m )
				groups.emplace_back ( sub.first, sub.second );

			out += replace ( groups );
			pos = m[ 0 ].second;
		}

		out.append ( pos, end );
		return out;
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up replacing in " << text.size () << " chars" );
		return std::string ( text );
	}
}
//-----------------------------------------------------------------------------

std::vector<regex::Pattern::Segment> regex::Pattern::segments ( const std::string_view text ) const
{
	std::vector<Segment>	out;

	auto	rest = [ &out ] ( const char* const from, const char* const to )
	{
		if ( from != to )
			out.push_back ( { std::string ( from, to ), false } );
	};

	const auto	begin = text.data () ? text.data () : "";
	const auto	end = begin + text.size ();

	if ( ! isValid () )
	{
		rest ( begin, end );
		return out;
	}

	try
	{
		auto	pos = begin;
		boost::match_results<const char*>	m;

		while ( pos != end && boost::regex_search ( pos, end, m, impl->re ) )
		{
			// An empty match would never advance; the remainder stays plain text
			if ( m[ 0 ].first == m[ 0 ].second )
				break;

			rest ( pos, m[ 0 ].first );
			out.push_back ( { std::string ( m[ 0 ].first, m[ 0 ].second ), true } );

			pos = m[ 0 ].second;
		}

		rest ( pos, end );
		return out;
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up splitting " << text.size () << " chars" );

		out.clear ();
		rest ( begin, end );
		return out;
	}
}
//-----------------------------------------------------------------------------

std::vector<std::string> regex::Pattern::capture ( const std::string_view text ) const
{
	if ( ! isValid () )
		return {};

	const auto	begin = text.data () ? text.data () : "";

	try
	{
		boost::match_results<const char*>	m;

		if ( ! boost::regex_search ( begin, begin + text.size (), m, impl->re ) )
			return {};

		std::vector<std::string>	groups;
		groups.reserve ( m.size () );

		// Unmatched optional groups come back as empty strings
		for ( const auto& sub : m )
			groups.emplace_back ( sub.first, sub.second );

		return groups;
	}
	catch ( ... )
	{
		Z_ERR ( "Regex gave up capturing from " << text.size () << " chars" );
		return {};
	}
}
//-----------------------------------------------------------------------------
