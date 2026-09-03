#include "ultra-shared/Config/DataSource.h"
#include "Shortcuts.h"

#include <algorithm>
#include <utility>

//-----------------------------------------------------------------------------

namespace
{
	struct Token
	{
		const char*	name;
		int			keyCode;	// 0 = a modifier
		int			modifier;
		const char*	text;		// the cap label
		const char*	svg;		// or its symbol
	};

	// The named tokens; single characters and F1..F12 are derived
	const Token	tokens[] = {
		{ "Ctrl",		0,								juce::ModifierKeys::commandModifier,	"Ctrl",		nullptr },
		{ "Alt",		0,								juce::ModifierKeys::altModifier,		"Alt",		nullptr },
		{ "Shift",		0,								juce::ModifierKeys::shiftModifier,		"Shift",	nullptr },
		{ "Space",		juce::KeyPress::spaceKey,		0,	"Space",	nullptr },
		{ "Enter",		juce::KeyPress::returnKey,		0,	nullptr,	"keycaps/enter" },
		{ "Escape",		juce::KeyPress::escapeKey,		0,	"Esc",		nullptr },
		{ "Tab",		juce::KeyPress::tabKey,			0,	"Tab",		nullptr },
		{ "Backspace",	juce::KeyPress::backspaceKey,	0,	"Backspace",nullptr },
		{ "Delete",		juce::KeyPress::deleteKey,		0,	"Del",		nullptr },
		{ "Home",		juce::KeyPress::homeKey,		0,	"Home",		nullptr },
		{ "End",		juce::KeyPress::endKey,			0,	"End",		nullptr },
		{ "PageUp",		juce::KeyPress::pageUpKey,		0,	"PgUp",		nullptr },
		{ "PageDown",	juce::KeyPress::pageDownKey,	0,	"PgDn",		nullptr },
		{ "Up",			juce::KeyPress::upKey,			0,	nullptr,	"keycaps/arrow-up" },
		{ "Down",		juce::KeyPress::downKey,		0,	nullptr,	"keycaps/arrow-down" },
		{ "Left",		juce::KeyPress::leftKey,		0,	nullptr,	"keycaps/arrow-left" },
		{ "Right",		juce::KeyPress::rightKey,		0,	nullptr,	"keycaps/arrow-right" },
	};

	const int	functionKeys[] = {
		juce::KeyPress::F1Key, juce::KeyPress::F2Key, juce::KeyPress::F3Key, juce::KeyPress::F4Key,
		juce::KeyPress::F5Key, juce::KeyPress::F6Key, juce::KeyPress::F7Key, juce::KeyPress::F8Key,
		juce::KeyPress::F9Key, juce::KeyPress::F10Key, juce::KeyPress::F11Key, juce::KeyPress::F12Key,
	};

	// The modifier glyphs the mac keyboard prints (commandModifier is the
	// JUCE meaning on both platforms: Cmd there, Ctrl on Windows/Linux)
	shortcuts::Cap modifierCap ( const Token& token )
	{
#if JUCE_MAC
		if ( token.modifier == juce::ModifierKeys::commandModifier )
			return { {}, "keycaps/command" };
		if ( token.modifier == juce::ModifierKeys::altModifier )
			return { {}, "keycaps/option" };
#endif
		return { token.text, {} };
	}
}
//-----------------------------------------------------------------------------

bool shortcuts::Chord::matches ( const juce::KeyPress& key ) const
{
	if ( typed != 0 )
		return key.getTextCharacter () == typed
			&& ! key.getModifiers ().withoutFlags ( juce::ModifierKeys::shiftModifier ).isAnyModifierKeyDown ();

	return key == press;
}
//-----------------------------------------------------------------------------

shortcuts::Chord shortcuts::parseChord ( const juce::String& text )
{
	Chord	chord;
	int		keyCode = 0;
	int		modifiers = 0;

	auto	parts = juce::StringArray::fromTokens ( text, "+", "" );
	parts.trim ();
	parts.removeEmptyStrings ();

	for ( const auto& part : parts )
	{
		if ( const auto* token = std::find_if ( std::begin ( tokens ), std::end ( tokens ), [ & ] ( const Token& t ) { return part == t.name; } ); token != std::end ( tokens ) )
		{
			if ( token->keyCode == 0 )
			{
				modifiers |= token->modifier;
				chord.caps.push_back ( modifierCap ( *token ) );
			}
			else
			{
				if ( keyCode != 0 || chord.typed != 0 )
					return {};

				keyCode = token->keyCode;
				chord.caps.push_back ( token->svg ? Cap { {}, token->svg } : Cap { token->text, {} } );
			}
		}
		else if ( part.length () > 1 && part[ 0 ] == 'F' && part.substring ( 1 ).containsOnly ( "0123456789" ) )
		{
			const auto	n = part.substring ( 1 ).getIntValue ();

			if ( keyCode != 0 || chord.typed != 0 || n < 1 || n > 12 )
				return {};

			keyCode = functionKeys[ n - 1 ];
			chord.caps.push_back ( { part, {} } );
		}
		else if ( part.length () == 1 )
		{
			if ( keyCode != 0 || chord.typed != 0 )
				return {};

			const auto	c = part[ 0 ];

			// Letters, digits and the unshifted punctuation of a US layout are
			// key codes; a shifted symbol ("?") is matched as typed, since the
			// key it sits on differs per layout
			if ( juce::CharacterFunctions::isLetterOrDigit ( c ) || juce::String ( ",./;'[]\\-=`" ).containsChar ( c ) )
				keyCode = static_cast<int> ( juce::CharacterFunctions::toUpperCase ( c ) );
			else
				chord.typed = c;

			chord.caps.push_back ( { part.toUpperCase (), {} } );
		}
		else
			return {};
	}

	if ( keyCode == 0 && chord.typed == 0 )
		return {};

	// A typed character carries whatever modifiers its layout needs
	if ( chord.typed != 0 && modifiers != 0 )
		return {};

	if ( keyCode != 0 )
		chord.press = juce::KeyPress ( keyCode, juce::ModifierKeys ( modifiers ), 0 );

	return chord;
}
//-----------------------------------------------------------------------------

Shortcuts::Shortcuts ()
{
	auto	header = true;

	for ( auto line : juce::StringArray::fromLines ( datasource::loadText ( "UI/shortcuts.csv" ) ) )
	{
		line = line.trim ();
		if ( line.isEmpty () || line.startsWithChar ( '#' ) )
			continue;

		// The first row names the columns
		if ( std::exchange ( header, false ) )
			continue;

		// keys is the last column and may itself hold a comma ("Ctrl+,"), so
		// only the first two commas separate
		const auto	section = line.upToFirstOccurrenceOf ( ",", false, false ).trim ();
		const auto	rest = line.fromFirstOccurrenceOf ( ",", false, false );
		const auto	verb = rest.upToFirstOccurrenceOf ( ",", false, false ).trim ();
		const auto	keys = rest.fromFirstOccurrenceOf ( ",", false, false ).trim ();

		if ( section.isEmpty () || verb.isEmpty () || keys.isEmpty () )
		{
			Z_ERR ( "shortcuts.csv: bad row: " << line );
			continue;
		}

		shortcuts::Entry	entry { section, verb, {} };

		for ( const auto& chordText : juce::StringArray::fromTokens ( keys, "|", "" ) )
		{
			auto	chord = shortcuts::parseChord ( chordText );

			if ( chord.caps.empty () )
				Z_ERR ( "shortcuts.csv: unknown chord '" << chordText << "' for " << verb );
			else if ( const auto bound = find ( chord.typed != 0 ? juce::KeyPress ( 0, juce::ModifierKeys::shiftModifier, chord.typed ) : chord.press ); bound.isNotEmpty () )
				Z_ERR ( "shortcuts.csv: '" << chordText << "' is bound to both " << bound << " and " << verb );
			else
				entry.chords.push_back ( std::move ( chord ) );
		}

		if ( ! entry.chords.empty () )
			entries.push_back ( std::move ( entry ) );
	}
}
//-----------------------------------------------------------------------------

juce::String Shortcuts::find ( const juce::KeyPress& key ) const
{
	for ( const auto& entry : entries )
		for ( const auto& chord : entry.chords )
			if ( chord.matches ( key ) )
				return entry.verb;

	return {};
}
//-----------------------------------------------------------------------------
