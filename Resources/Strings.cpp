#include "ultra-shared/Config/DataSource.h"
#include "Strings.h"

//-----------------------------------------------------------------------------

Strings::Strings ()
{
	load ();
}
//-----------------------------------------------------------------------------

void Strings::setLanguage ( const juce::String& _language )
{
	if ( language == _language )
		return;

	language = _language;
	load ();
}
//-----------------------------------------------------------------------------

void Strings::load ()
{
	loadText ( datasource::loadText ( "UI/strings/" + language + ".yml" ) );

	// The CRT-settings strings ship as their own fragment (owned by the
	// ultra-shared submodule) layered over the language file
	if ( datasource::exists ( "UI/strings/crt-settings.yml" ) )
		appendText ( datasource::loadText ( "UI/strings/crt-settings.yml" ) );
}
//-----------------------------------------------------------------------------

juce::String Strings::getOptional ( const juce::String& name, const juce::String& fallback ) const
{
	const auto	it = result.find ( name );

	return it != result.end () ? it->second : fallback;
}
//-----------------------------------------------------------------------------

const juce::String& Strings::get ( const juce::String& name )
{
	if ( auto it = result.find ( name ); it != result.end () )
		return it->second;

	// Key missing from the language file, show (and cache) the key itself,
	// warn once per run so typos surface in the log
	Z_WARN ( "Missing UI string: " << name );

	result.insert ( { name, name } );

	return result[ name ];
}
//-----------------------------------------------------------------------------
