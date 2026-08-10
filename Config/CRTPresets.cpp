#include <JuceHeader.h>

#include "ultra-shared/Config/CRTPresets.h"

#include "ultra-shared/Config/DataSource.h"
#include "ultra-shared/Helpers/FileUtils.h"
#include "Config/FilePaths.h"

//-----------------------------------------------------------------------------

namespace
{
	enum class kind { number, decimal, pair, name };

	struct scopeEntry
	{
		const char*	key;
		kind		k;
		bool		saved = true;	// false: never captured by saving, only hand-authored files pin it
	};

	// The preset scope in panel order: jailbars through reflections;
	// everything outside the crt section stays personal. Presets carry only
	// the keys present in their file; missing keys leave the live value
	// alone and stay out of the Custom comparison. The unsaved keys are
	// personal taste (reflection) or chip artifacts (jailbars): saving never
	// captures them, but a hand-authored preset may pin them — an
	// emulator-look preset (VICE, C64 Ultimate) carries jailbars: 0 because
	// those don't emulate it
	constexpr scopeEntry	presetScope[] =
	{
		{ "jailbars",		kind::number,	false },

		{ "noise",			kind::number },
		{ "sharpening",		kind::number },
		{ "luma-blur",		kind::number },
		{ "chroma-blur",	kind::number },
		{ "crosstalk",		kind::number },
		{ "phase",			kind::decimal },
		{ "hannover",		kind::number },
		{ "rainbowing",		kind::number },
		{ "drift",			kind::number },

		{ "curve",			kind::number },
		{ "rotation",		kind::number },
		{ "bleed",			kind::number },
		{ "bleed-red",		kind::pair },
		{ "bleed-green",	kind::pair },
		{ "bleed-blue",		kind::pair },
		{ "convergence",	kind::number },
		{ "h-wave",			kind::number },
		{ "expansion",		kind::number },

		{ "scanlines",		kind::number },
		{ "mask",			kind::number },
		{ "mask-bitmap",	kind::name },
		{ "phosphor-decay",	kind::number },

		{ "vignette",		kind::number },
		{ "adjacent",		kind::number },
		{ "halation",		kind::number },
		{ "ambient",		kind::number },
		{ "reflection",		kind::number,	false },
	};
	//-------------------------------------------------------------------------

	[[ nodiscard ]] std::vector<YamlFile::value> scopeDefaults ()
	{
		std::vector<YamlFile::value>	out;
		out.reserve ( std::size ( presetScope ) );

		for ( const auto& s : presetScope )
			switch ( s.k )
			{
				case kind::number:	out.push_back ( { "crt", s.key, 0 } );						break;
				case kind::decimal:	out.push_back ( { "crt", s.key, 0.0f } );					break;
				case kind::pair:	out.push_back ( { "crt", s.key, YamlFile::vec2i {} } );		break;
				case kind::name:	out.push_back ( { "crt", s.key, std::string () } );			break;
			}

		return out;
	}
	//-------------------------------------------------------------------------

	// Declares the scope keys so get<T> parses them typed; presence of a key
	// in the file is what makes it part of the preset
	class PresetFile final : public YamlFile
	{
	public:
		PresetFile () : YamlFile ( scopeDefaults (), {}, true ) {}

		using YamlFile::loadText;

		[[ nodiscard ]] bool has ( const juce::String& path ) const	{	return result.find ( path ) != result.end ();	}
	};
	//-------------------------------------------------------------------------

	// Mask names live in the marked pick-list form ("$DATA$/Slot Mask"); an
	// unmarked value counts as factory
	[[ nodiscard ]] juce::String markedMaskName ( const juce::String& value )
	{
		return value.startsWith ( "$" ) ? value : filepaths::markerFor ( filepaths::root::data ) + "/" + value;
	}
	//-------------------------------------------------------------------------

	[[ nodiscard ]] bool maskBitmapExists ( const juce::String& markedMask )
	{
		const auto	plain = markedMask.fromFirstOccurrenceOf ( "/", false, false );

		if ( markedMask.startsWith ( filepaths::markerFor ( filepaths::root::user ) ) )
			return filepaths::getUserCRTMasksPath ().getChildFile ( plain + ".png" ).existsAsFile ();

		return datasource::exists ( "CRTEmulation/CRT Masks/" + plain + ".png" );
	}
}
//-----------------------------------------------------------------------------

juce::StringArray crtpresets::listPresets ()
{
	const auto	dataMarker = filepaths::markerFor ( filepaths::root::data ) + "/";
	const auto	userMarker = filepaths::markerFor ( filepaths::root::user ) + "/";

	juce::StringArray	factory;
	for ( const auto& name : datasource::listFiles ( "CRTEmulation/Presets", false, "*.yml" ) )
		factory.add ( name.upToLastOccurrenceOf ( ".", false, false ) );
	factory.sortNatural ();

	// Default leads the factory list
	if ( const auto idx = factory.indexOf ( "Default" ); idx > 0 )
		factory.move ( idx, 0 );

	juce::StringArray	user;
	if ( const auto root = filepaths::getUserCRTPresetsPath (); root != juce::File () )
		for ( const auto& f : root.findChildFiles ( juce::File::findFiles | juce::File::ignoreHiddenFiles, false, "*.yml" ) )
			user.add ( f.getFileNameWithoutExtension () );
	user.sortNatural ();

	juce::StringArray	out;

	for ( const auto& name : factory )
		out.add ( dataMarker + name );

	for ( const auto& name : user )
		out.add ( userMarker + name );

	return out;
}
//-----------------------------------------------------------------------------

// Developer authoring goes straight into the factory data; a naked data root
// only ever boots in developer mode, so unpaked = the developer checkout
static bool savesToFactory ()
{
	return ! datasource::isPak ();
}
//-----------------------------------------------------------------------------

juce::File crtpresets::saveTargetFile ( const juce::String& name )
{
	if ( savesToFactory () )
		return datasource::getDevFile ( "CRTEmulation/Presets/" + name + ".yml" );

	if ( const auto root = filepaths::getUserCRTPresetsPath (); root != juce::File () )
		return root.getChildFile ( name + ".yml" );

	return {};
}
//-----------------------------------------------------------------------------

juce::String crtpresets::saveCurrentValues ( const Preferences& preferences, const juce::String& name )
{
	const auto	file = saveTargetFile ( name );
	if ( file == juce::File () )
		return {};

	juce::String	out ( "crt:\n" );

	for ( const auto& s : presetScope )
	{
		if ( ! s.saved )
			continue;

		const auto	path = "crt/" + juce::String ( s.key );

		juce::String	str;
		switch ( s.k )
		{
			case kind::number:
				str = juce::String ( preferences.get<int> ( path ) );
				break;

			case kind::decimal:
				str = juce::String ( preferences.get<float> ( path ) );
				break;

			case kind::pair:
			{
				const auto	v = preferences.get<YamlFile::vec2i> ( path );
				str = juce::String ( v.first ) + ", " + juce::String ( v.second );
				break;
			}

			case kind::name:
				str = markedMaskName ( preferences.get<juce::String> ( path ) );
				break;
		}

		out += "  " + juce::String ( s.key ) + ": " + str + "\n";
	}

	if ( ! fileutils::replaceFile ( file, out ) )
		return {};

	return filepaths::markerFor ( savesToFactory () ? filepaths::root::data : filepaths::root::user ) + "/" + name;
}
//-----------------------------------------------------------------------------

void CRTPreset::load ( const juce::String& markedName )
{
	valid = false;
	marked = markedName;
	entries.clear ();

	if ( markedName.isEmpty () )
		return;

	const auto	plain = markedName.startsWith ( "$" ) ? markedName.fromFirstOccurrenceOf ( "/", false, false ) : markedName;

	const auto	text = markedName.startsWith ( filepaths::markerFor ( filepaths::root::user ) )
		? filepaths::getUserCRTPresetsPath ().getChildFile ( plain + ".yml" ).loadFileAsString ()
		: datasource::loadText ( "CRTEmulation/Presets/" + plain + ".yml" );

	if ( text.isEmpty () )
		return;

	PresetFile	file;
	file.loadText ( text );

	for ( const auto& s : presetScope )
	{
		const auto	path = "crt/" + juce::String ( s.key );

		if ( ! file.has ( path ) )
			continue;

		switch ( s.k )
		{
			case kind::number:	entries.push_back ( { path, file.get<int> ( path ) } );					break;
			case kind::decimal:	entries.push_back ( { path, file.get<float> ( path ) } );				break;
			case kind::pair:	entries.push_back ( { path, file.get<YamlFile::vec2i> ( path ) } );		break;
			case kind::name:	entries.push_back ( { path, file.get<std::string> ( path ) } );			break;
		}
	}

	valid = ! entries.empty ();
}
//-----------------------------------------------------------------------------

void CRTPreset::applyTo ( Preferences& preferences ) const
{
	for ( const auto& e : entries )
	{
		if ( const auto* n = std::get_if<int> ( &e.value ) )
		{
			preferences.set ( e.path, *n );
		}
		else if ( const auto* d = std::get_if<float> ( &e.value ) )
		{
			preferences.set ( e.path, *d );
		}
		else if ( const auto* p = std::get_if<YamlFile::vec2i> ( &e.value ) )
		{
			preferences.set ( e.path, *p );
		}
		else if ( const auto* s = std::get_if<std::string> ( &e.value ) )
		{
			if ( const auto markedMask = markedMaskName ( juce::String ( *s ) ); maskBitmapExists ( markedMask ) )
				preferences.set ( e.path, markedMask );
		}
	}
}
//-----------------------------------------------------------------------------

bool CRTPreset::matches ( const Preferences& preferences ) const
{
	if ( ! valid )
		return false;

	for ( const auto& e : entries )
	{
		if ( const auto* n = std::get_if<int> ( &e.value ) )
		{
			if ( preferences.get<int> ( e.path ) != *n )
				return false;
		}
		else if ( const auto* d = std::get_if<float> ( &e.value ) )
		{
			if ( preferences.get<float> ( e.path ) != *d )
				return false;
		}
		else if ( const auto* p = std::get_if<YamlFile::vec2i> ( &e.value ) )
		{
			if ( preferences.get<YamlFile::vec2i> ( e.path ) != *p )
				return false;
		}
		else if ( const auto* s = std::get_if<std::string> ( &e.value ) )
		{
			if ( markedMaskName ( preferences.get<juce::String> ( e.path ) ) != markedMaskName ( juce::String ( *s ) ) )
				return false;
		}
	}

	return true;
}
//-----------------------------------------------------------------------------
