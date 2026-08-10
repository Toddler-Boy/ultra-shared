#pragma once

#include <JuceHeader.h>

#include <algorithm>
#include <variant>

#include "ultra-shared/Helpers/FileUtils.h"

//-----------------------------------------------------------------------------

// Yaml-backed key/value store.
//
// Read-only use (Strings, Icons, Theme): default-construct, set `file`, call
// load(), values are available in `result` as "path/joined/keys" (lowercased).
//
// Read-write use (Settings, Preferences): construct with a table of typed
// default values, then use get<T>/set<T> with a combined "section/key" name.
// load() parses each known key ONCE into its declared variant type; get/set
// are then just an index lookup plus a compile-time coercion (so e.g.
// get<float> of an int-declared key still works). Changed values are written
// back on destruction as a complete, canonical two-level yaml file in table
// order.

class YamlFile
{
public:
	using vec2i = std::pair<int, int>;
	using vec2f = std::pair<float, float>;

	using ConfigValue = std::variant<bool, float, double, int, std::string, vec2i, vec2f>;

	struct value
	{
		std::string	section;
		std::string	key;
		ConfigValue	defaultValue;
	};

	YamlFile () = default;

	YamlFile ( const std::vector<value>& defaultValues, const juce::File& _file = {}, const bool _readOnly = false )
		: readOnly ( _readOnly )
		, values ( defaultValues )
	{
		current.reserve ( values.size () );
		lookup.reserve ( values.size () );

		for ( const auto& val : values )
		{
			current.push_back ( val.defaultValue );
			lookup.push_back ( ( juce::String ( val.section ) + "/" + juce::String ( val.key ) ).toLowerCase () );
		}

		if ( _file != juce::File () )
			load ( _file );
	}
	//-------------------------------------------------------------------------

	virtual ~YamlFile ()
	{
		save ();
	}
	//-------------------------------------------------------------------------

	[[ nodiscard ]] bool hasData () const
	{
		return ! result.empty ();
	}
	//-------------------------------------------------------------------------

	virtual void load ()
	{
		result.clear ();
		dirty = false;

		parse ( file.loadFileAsString () );
		applyTyped ();
	}
	//-------------------------------------------------------------------------

	// For content that doesn't live in a real file (factory data from the pak)
	void loadText ( const juce::String& text )
	{
		result.clear ();
		dirty = false;

		parse ( text );
		applyTyped ();
	}
	//-------------------------------------------------------------------------

	// Merges more keys into the already-loaded set (layered string files);
	// duplicate paths keep the appended value
	void appendText ( const juce::String& text )
	{
		parse ( text );
		applyTyped ();
	}
	//-------------------------------------------------------------------------

	void load ( const juce::File& _file )
	{
		// Anything still pending belongs to the file we are leaving
		save ();

		file = _file;
		file.create ();

		load ();
	}
	//-----------------------------------------------------------------------------

	// Writes all known values (defaults table order) as a two-level yaml file
	void save ()
	{
		if ( readOnly || ! dirty || file == juce::File () || values.empty () )
			return;

		juce::String	out;
		juce::String	lastSection;

		for ( auto i = 0; i < int ( values.size () ); ++i )
		{
			const auto	section = juce::String ( values[ i ].section ).toLowerCase ();
			const auto	key = juce::String ( values[ i ].key ).toLowerCase ();

			if ( section != lastSection )
			{
				if ( out.isNotEmpty () )
					out += "\n";

				out += section + ":\n";
				lastSection = section;
			}

			auto	str = valueToString ( current[ i ] );

			// Quote values the parser would otherwise mangle (empty, padded,
			// quote-bounded, or a lone "|" that the reader takes for a block
			// scalar; the load-side unquoted () strips the added pair)
			if ( str.isEmpty () || str != str.trim () || str.startsWithChar ( '"' ) || str.endsWithChar ( '"' ) || str == "|" )
				str = "\"" + str + "\"";

			out += "  " + key + ": " + str + "\n";
		}

		if ( ! fileutils::replaceFile ( file, out ) )
			return;

		dirty = false;
	}
	//-----------------------------------------------------------------------------

	// The typed accessors live further down, their helpers must be declared
	// first, or two-phase template lookup rejects the calls.

private:
	[[ nodiscard ]] int findIndex ( const juce::String& sectionKey ) const
	{
		jassert ( sectionKey.containsChar ( '/' ) ); // Must be in "section/key" format

		// Stored keys are lowercased at load and NOT lowercased here, an
		// uppercase character in the lookup would silently fail (keys are
		// plain ascii, so a simple range check suffices)
		jassert ( ! std::any_of ( sectionKey.begin (), sectionKey.end (),
								  [] ( const auto c ) { return c >= 'A' && c <= 'Z'; } ) );

		for ( auto i = 0; i < int ( lookup.size () ); ++i )
			if ( lookup[ i ] == sectionKey )
				return i;

		// That section/key doesn't exist! Typo?
		jassertfalse;
		return -1;
	}
	//-----------------------------------------------------------------------------

	template<typename T>
	static constexpr bool isPair = std::is_same_v<T, vec2i> || std::is_same_v<T, vec2f>;

	template<typename T>
	[[ nodiscard ]] static T parsePair ( const juce::String& str )
	{
		auto	parts = juce::StringArray::fromTokens ( str, ",", "" );

		parts.trim ();
		parts.removeEmptyStrings ();

		if ( parts.size () != 2 )
			return T {};

		if constexpr ( std::is_same_v<T, vec2i> )
			return { parts[ 0 ].getIntValue (), parts[ 1 ].getIntValue () };
		else
			return { parts[ 0 ].getFloatValue (), parts[ 1 ].getFloatValue () };
	}
	//-----------------------------------------------------------------------------

	[[ nodiscard ]] static juce::String valueToString ( const ConfigValue& val )
	{
		return std::visit ( [] ( const auto& v ) -> juce::String
		{
			using E = std::decay_t<decltype ( v )>;

			if constexpr ( std::is_same_v<E, bool> )
				return v ? "true" : "false";
			else if constexpr ( std::is_same_v<E, float> || std::is_same_v<E, double> )
				return juce::String ( v, 0 );
			else if constexpr ( std::is_same_v<E, int> )
				return juce::String ( v );
			else if constexpr ( std::is_same_v<E, std::string> )
				return juce::String ( v );
			else	// vec2i / vec2f
				return juce::String ( v.first ) + ", " + juce::String ( v.second );
		}, val );
	}
	//-----------------------------------------------------------------------------

	// Extract as T from whatever alternative is stored, converting when sensible.
	// The T-branches live OUTSIDE the visit lambdas: inside them T is fixed, so
	// T-only constructs would be type-checked even in discarded branches.
	template<typename T>
	[[ nodiscard ]] static T convertTo ( const ConfigValue& val )
	{
		if constexpr ( std::is_same_v<T, juce::String> )
			return valueToString ( val );
		else if constexpr ( std::is_same_v<T, std::string> )
			return valueToString ( val ).toStdString ();
		else if constexpr ( isPair<T> )
		{
			return std::visit ( [] ( const auto& v ) -> T
			{
				using E = std::decay_t<decltype ( v )>;

				if constexpr ( std::is_same_v<E, T> )
					return v;
				else if constexpr ( isPair<E> )
					return { static_cast<typename T::first_type> ( v.first ), static_cast<typename T::second_type> ( v.second ) };
				else if constexpr ( std::is_same_v<E, std::string> )
					return parsePair<T> ( juce::String ( v ) );
				else
				{
					jassertfalse;	// Unsupported conversion
					return T {};
				}
			}, val );
		}
		else
		{
			static_assert ( std::is_arithmetic_v<T>, "Unsupported type in YamlFile::get" );

			return std::visit ( [] ( const auto& v ) -> T
			{
				using E = std::decay_t<decltype ( v )>;

				if constexpr ( std::is_arithmetic_v<E> )
					return static_cast<T> ( v );
				else if constexpr ( std::is_same_v<E, std::string> )
				{
					const juce::String	str ( v );

					if constexpr ( std::is_same_v<T, bool> )
						return str.equalsIgnoreCase ( "true" );
					else
						return static_cast<T> ( str.getDoubleValue () );
				}
				else
				{
					jassertfalse;	// Unsupported conversion (pair -> number)
					return T {};
				}
			}, val );
		}
	}
	//-----------------------------------------------------------------------------

	// Wrap a caller-supplied value into its natural variant alternative
	template<typename T>
	[[ nodiscard ]] static ConfigValue makeValue ( const T& v )
	{
		if constexpr ( std::is_same_v<T, bool> )
			return v;
		else if constexpr ( std::is_integral_v<T> )
			return int ( v );
		else if constexpr ( std::is_same_v<T, double> )
			return v;
		else if constexpr ( std::is_floating_point_v<T> )
			return float ( v );
		else if constexpr ( isPair<T> )
			return v;
		else if constexpr ( std::is_same_v<T, std::string> )
			return v;
		else if constexpr ( std::is_constructible_v<juce::String, T> )
			return juce::String ( v ).toStdString ();
		else
			static_assert ( false, "Unsupported type in YamlFile::set" );
	}
	//-----------------------------------------------------------------------------

	[[ nodiscard ]] static ConfigValue coerceToDeclared ( const ConfigValue& declared, const ConfigValue& v )
	{
		return std::visit ( [ &v ] ( const auto& d ) -> ConfigValue
		{
			using E = std::decay_t<decltype ( d )>;
			return convertTo<E> ( v );
		}, declared );
	}
	//-----------------------------------------------------------------------------

public:
	template<typename T>
	[[ nodiscard ]] T get ( const juce::String& sectionKey ) const
	{
		const auto	idx = findIndex ( sectionKey );
		if ( idx < 0 )
			return T {};

		return convertTo<T> ( current[ idx ] );
	}
	//-----------------------------------------------------------------------------

	template<typename T>
	[[ nodiscard ]] T getDefault ( const juce::String& sectionKey ) const
	{
		const auto	idx = findIndex ( sectionKey );
		if ( idx < 0 )
			return T {};

		return convertTo<T> ( values[ idx ].defaultValue );
	}
	//-----------------------------------------------------------------------------

	template<typename T>
	void set ( const juce::String& sectionKey, const T value )
	{
		const auto	idx = findIndex ( sectionKey );
		if ( idx < 0 )
			return;

		// Coerce to the declared type so the variant alternative never drifts
		auto	nv = coerceToDeclared ( values[ idx ].defaultValue, makeValue ( value ) );

		dirty = dirty || current[ idx ] != nv;
		current[ idx ] = std::move ( nv );
	}
	//-----------------------------------------------------------------------------

protected:
	juce::File	file;
	std::unordered_map<juce::String, juce::String>	result;

private:
	// True when the file's string is a plausible value of the declared type
	[[ nodiscard ]] static bool parsesAsDeclared ( const ConfigValue& declared, const juce::String& str )
	{
		return std::visit ( [ &str ] ( const auto& d ) -> bool
		{
			using E = std::decay_t<decltype ( d )>;

			const auto	t = str.trim ();

			if constexpr ( std::is_same_v<E, bool> )
				return t.equalsIgnoreCase ( "true" ) || t.equalsIgnoreCase ( "false" );
			else if constexpr ( std::is_arithmetic_v<E> )
				return t.isNotEmpty () && t.containsOnly ( "+-.0123456789eE" ) && t.containsAnyOf ( "0123456789" );
			else
				return true;
		}, declared );
	}
	//-----------------------------------------------------------------------------

	// Convert the parsed strings of all known keys into their declared types.
	// Keys missing from the file, or with a value that doesn't parse as the
	// declared type, fall back to their defaults and mark the store dirty,
	// so the file is completed on the next save.
	void applyTyped ()
	{
		for ( auto i = 0; i < int ( values.size () ); ++i )
		{
			const auto	it = result.find ( lookup[ i ] );

			if ( it == result.end () || ! parsesAsDeclared ( values[ i ].defaultValue, it->second ) )
			{
				current[ i ] = values[ i ].defaultValue;
				dirty = true;
				continue;
			}

			current[ i ] = coerceToDeclared ( values[ i ].defaultValue, ConfigValue { it->second.toStdString () } );
		}
	}
	//-----------------------------------------------------------------------------

	void parse ( const juce::String& input )
	{
		juce::StringArray pathStack;
		juce::Array<int> indentStack;

		const auto lines = juce::StringArray::fromLines ( input );

		for ( auto i = 0; i < lines.size (); ++i )
		{
			const auto&	line = lines[ i ];

			if ( line.trim ().isEmpty () || line.trim ().startsWith ( "#" ) )
				continue;

			const auto	indent = line.initialSectionContainingOnly ( " \t\r\n" ).length ();

			// Adjust path based on indentation
			while ( ! indentStack.isEmpty () && indentStack.getLast () >= indent )
			{
				indentStack.removeLast ();
				pathStack.remove ( pathStack.size () - 1 );
			}

			if ( line.containsChar ( ':' ) )
			{
				const auto	key = line.upToFirstOccurrenceOf ( ":", false, false ).trim ().toLowerCase ();
				const auto	val = line.fromFirstOccurrenceOf ( ":", false, false ).trim ();

				pathStack.add ( key );
				indentStack.add ( indent );
				const auto fullPath = pathStack.joinIntoString ( "/" );

				if ( val == "|" )
				{
					juce::String block;
					while ( ++i < lines.size () )
					{
						const auto&	next = lines[ i ];

						if ( next.trim ().isEmpty () )
						{
							block << "\n";
							continue;
						}

						if ( next.initialSectionContainingOnly ( " \t" ).length () <= indent )
						{
							i--;
							break; // Backtrack for main loop
						}
						block << next.trim () << "\n";
					}
					result[ fullPath ] = block;
				}
				else if ( val.isNotEmpty () )
				{
					result[ fullPath ] = val.unquoted ();
				}
			}
		}
	}

	bool	dirty = false;
	bool	readOnly = false;

	const std::vector<value>	values;
	std::vector<ConfigValue>	current;
	std::vector<juce::String>	lookup;	// lowercased "section/key" per entry
};
//-----------------------------------------------------------------------------
