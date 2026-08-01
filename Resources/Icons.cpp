#include "ultra-shared/Config/DataSource.h"
#include "Icons.h"

//-----------------------------------------------------------------------------

Icons::Icons ()
{
	load ();
}
//-----------------------------------------------------------------------------

void Icons::load ()
{
	loadText ( datasource::loadText ( "UI/icons.yml" ) );

	// Replace missing icons with "notdef" (a blank square)
	for ( auto& ent : result )
		if ( ! datasource::exists ( "UI/svg/" + ent.second + ".svg" ) )
			ent.second = "font-awesome/notdef-solid-full";
}
//-----------------------------------------------------------------------------

const juce::String& Icons::get ( const juce::String& name )
{
	if ( auto it = result.find ( name ); it != result.end () )
		return it->second;

	result.insert ( { name, "font-awesome/bug-solid-full" } );

	return result[ name ];
}
//-----------------------------------------------------------------------------
