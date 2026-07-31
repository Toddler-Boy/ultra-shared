#include <JuceHeader.h>

#include <algorithm>
#include <map>
#include <unordered_map>
#include <utility>

#include "Theme.h"

#include "Config/DataSource.h"
#include "Config/FilePaths.h"
#include "UI/ui-colors.h"

//-----------------------------------------------------------------------------

// Keyed by "block/key" theme paths: the global palette lives under
// "colors/", component-specific colors live inside their component's block.
// The host app's COLOR_ROLES registry supplies the entries
static const std::map<juce::String, std::pair<int, juce::Colour>>	colorDefinitions =
{
	#define X(role, name, col) { name, { UI::colors::role, col } },
	COLOR_ROLES ( X )
	#undef X
};
//-----------------------------------------------------------------------------

Theme::Theme ()
{
	// A duplicate theme path in the registry would silently collapse map entries
	jassert ( colorDefinitions.size () == size_t ( UI::colors::count - UI::colors::window ) );

	resetDefaults ();
}
//-----------------------------------------------------------------------------

void Theme::resetDefaults ()
{
	cornerRadius = std::array<float, static_cast<size_t> ( UI::corners::count )>
	{
		#define X(role, name, r) r,
		CORNER_ROLES ( X )
		#undef X
	};

	lineWidths = std::array<float, static_cast<size_t> ( UI::lines::count )>
	{
		#define X(role, name, w) w,
		LINE_ROLES ( X )
		#undef X
	};

	paddingDefs = std::array<UI::paddings::Def, static_cast<size_t> ( UI::paddings::count )>
	{ {
		#define X(role, name, p) UI::paddings::Def { p, p, p, p },
		PADDING_ROLES ( X )
		#undef X
	} };

	fontDefs = std::array<UI::fonts::Def, static_cast<size_t> ( UI::fonts::count )>
	{ {
		#define X(role, name, h, w) UI::fonts::Def { h, w },
		FONT_ROLES ( X )
		#undef X
	} };
}
//-----------------------------------------------------------------------------

void Theme::load ( const juce::String& name )
{
	// This needs a target juce::LookAndFeel to work
	jassert ( laf );

	auto	themeMap = std::unordered_map<juce::String, juce::Colour> {};

	resetDefaults ();

	// Factory themes come through datasource, user themes are real files
	auto	loaded = false;

	if ( name.startsWith ( filepaths::markerFor ( filepaths::root::user ) ) )
	{
		if ( const auto themeFile = resolve ( name ); themeFile.existsAsFile () )
		{
			file = themeFile;
			YamlFile::load ();
			loaded = true;
		}
	}
	else if ( const auto text = datasource::loadText ( "UI/themes/" + name.fromFirstOccurrenceOf ( "/", false, false ) + ".yml" ); text.isNotEmpty () )
	{
		loadText ( text );
		loaded = true;
	}

	if ( loaded )
	{
		auto getFileColor = [] ( juce::String colStr )
		{
			// Named color
			auto	col = juce::Colours::findColourForName ( colStr, juce::Colours::transparentWhite );
			if ( col != juce::Colours::transparentWhite )
				return col;

			// Not correct format?
			if ( ! colStr.startsWithChar ( '#' ) )
				return juce::Colours::transparentWhite;

			colStr = colStr.substring ( 1 );
			if ( ! colStr.containsOnly ( "0123456789abcdef" ) )
				return juce::Colours::transparentWhite;

			// Swap alpha into the right position
			if ( colStr.length () == 3 )
				colStr = juce::String::formatted ( "ff%c%c%c%c%c%c", colStr[ 0 ], colStr[ 0 ], colStr[ 1 ], colStr[ 1 ], colStr[ 2 ], colStr[ 2 ] );
			else if ( colStr.length () == 4 )
				colStr = juce::String::formatted ( "%c%c%c%c%c%c%c%c", colStr[ 3 ], colStr[ 3 ], colStr[ 0 ], colStr[ 0 ], colStr[ 1 ], colStr[ 1 ], colStr[ 2 ], colStr[ 2 ] );
			else if ( colStr.length () == 6 )
				colStr = "ff" + colStr;
			else if ( colStr.length () == 8 )
				colStr = colStr.substring ( 6, 6 + 2 ) + colStr.substring ( 0, 6 );
			else
				return juce::Colours::transparentWhite;

			return juce::Colour::fromString ( colStr );
		};

		// Every key is a "block/key" path: a block groups all of a
		// component's properties (CSS-class style), and the path alone
		// picks the registry - fonts, corners, lines or colors
		for ( const auto& [ path, rawValue ] : result )
		{
			const auto	value = rawValue.toLowerCase ();

			if ( const auto fontRole = UI::fonts::fromName ( path ); fontRole != UI::fonts::count )
			{
				// "point-size weight", e.g. "13 500"
				const auto	tokens = juce::StringArray::fromTokens ( value, " ", "" );

				const auto	size = tokens[ 0 ].getFloatValue ();
				const auto	weight = tokens[ 1 ].getIntValue ();

				if ( size > 0.0f && weight >= 100 && weight <= 1000 )
					fontDefs[ size_t ( fontRole ) ] = { size, weight };
				else
					Z_ERR ( "Bad font definition (" << path << ": " << rawValue << ") in theme" );
			}
			else if ( const auto cornerRole = UI::corners::fromName ( path ); cornerRole != UI::corners::count )
			{
				if ( const auto radius = value.getFloatValue (); radius >= 0.0f )
					cornerRadius[ size_t ( cornerRole ) ] = radius;
				else
					Z_ERR ( "Bad corner radius (" << path << ": " << rawValue << ") in theme" );
			}
			else if ( const auto lineRole = UI::lines::fromName ( path ); lineRole != UI::lines::count )
			{
				// 0 is allowed: it hides the line entirely
				if ( const auto width = value.getFloatValue (); width >= 0.0f )
					lineWidths[ size_t ( lineRole ) ] = width;
				else
					Z_ERR ( "Bad line width (" << path << ": " << rawValue << ") in theme" );
			}
			else if ( const auto paddingRole = UI::paddings::fromName ( path ); paddingRole != UI::paddings::count )
			{
				// CSS shorthand: "all", "tb lr", "top lr bottom" or
				// "top right bottom left"; negatives grow the rect
				const auto	tokens = juce::StringArray::fromTokens ( value, " ", "" );

				const auto	numeric = [] ( const juce::String& t )
				{
					return t.containsOnly ( "+-.0123456789" ) && t.containsAnyOf ( "0123456789" );
				};

				if ( tokens.size () >= 1 && tokens.size () <= 4
					 && std::all_of ( tokens.begin (), tokens.end (), numeric ) )
				{
					const auto	v = [ &tokens ] ( const int i ) { return tokens[ i ].getFloatValue (); };

					auto&	def = paddingDefs[ size_t ( paddingRole ) ];

					switch ( tokens.size () )
					{
						case 1:	def = { v ( 0 ), v ( 0 ), v ( 0 ), v ( 0 ) };	break;
						case 2:	def = { v ( 0 ), v ( 1 ), v ( 0 ), v ( 1 ) };	break;
						case 3:	def = { v ( 0 ), v ( 1 ), v ( 2 ), v ( 1 ) };	break;
						default: def = { v ( 0 ), v ( 1 ), v ( 2 ), v ( 3 ) };	break;
					}
				}
				else
				{
					Z_ERR ( "Bad padding (" << path << ": " << rawValue << ") in theme" );
				}
			}
			else if ( colorDefinitions.contains ( path ) )
			{
				// Store valid colors in themeMap
				if ( const auto col = getFileColor ( value ); col != juce::Colours::transparentWhite )
					themeMap[ path ] = col;
			}
			else
			{
				Z_ERR ( "Unknown theme key (" << path << ")" );
			}
		}
	}

	// Apply colors
	{
		for ( const auto& [ colName, idDef ] : colorDefinitions )
			if ( themeMap.contains ( colName ) )
				laf->setColour ( idDef.first, themeMap[ colName ] );
			else
				laf->setColour ( idDef.first, idDef.second );
	}
}
//-----------------------------------------------------------------------------

void Theme::setUserRoot ( const juce::File& _userRoot )
{
	if ( _userRoot.isDirectory () )
		userRoot = _userRoot;
	else
		userRoot = juce::File ();
}
//-----------------------------------------------------------------------------

juce::File Theme::resolve ( const juce::String& markedName ) const
{
	const auto	name = markedName.fromFirstOccurrenceOf ( "/", false, false ) + ".yml";

	if ( markedName.startsWith ( filepaths::markerFor ( filepaths::root::user ) ) )
		return userRoot == juce::File () ? juce::File () : userRoot.getChildFile ( name );

	// Factory themes only exist as real files in the naked layout; the watcher
	// comparing against this never runs in pak mode
	if ( datasource::isPak () )
		return {};

	return datasource::getDevFile ( "UI/themes/" + name );
}
//-----------------------------------------------------------------------------

juce::StringArray Theme::listThemes () const
{
	juce::StringArray	out;

	for ( const auto which : { filepaths::root::data, filepaths::root::user } )
	{
		juce::StringArray	names;

		if ( which == filepaths::root::user )
		{
			for ( const auto& f : userRoot.findChildFiles ( juce::File::findFiles, false, "*.yml" ) )
				names.add ( f.getFileNameWithoutExtension () );
		}
		else
		{
			for ( const auto& f : datasource::listFiles ( "UI/themes/", false, "*.yml" ) )
				names.add ( f.upToLastOccurrenceOf ( ".", false, false ) );
		}

		names.sortNatural ();

		for ( const auto& name : names )
			out.add ( filepaths::markerFor ( which ) + "/" + name );
	}

	return out;
}
//-----------------------------------------------------------------------------

#undef COLOR_ROLES
#undef CORNER_ROLES
#undef FONT_ROLES
#undef LINE_ROLES
#undef PADDING_ROLES
