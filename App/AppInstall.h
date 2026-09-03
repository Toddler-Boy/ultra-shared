#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// Windows integration for the portable exe, all without elevation: a Start
// menu shortcut to the running program, and a move of the program into the
// per-user programs folder (%LOCALAPPDATA%\Programs\<app>)

namespace appinstall
{
	#if JUCE_WINDOWS
		constexpr bool	supported = true;
	#else
		constexpr bool	supported = false;
	#endif

	// Where the move puts the program
	[[ nodiscard ]] juce::File programsFolderExe ();
	[[ nodiscard ]] bool runsFromProgramsFolder ();

	// Writes (or rewrites) the Start menu entry for the running program
	bool createStartMenuShortcut ();

	// Moves the running program into the programs folder, writes the Start
	// menu entry for it and arranges the relaunch from there; the caller quits
	bool moveToProgramsFolder ();

	// A move across volumes leaves the original behind until this next start
	void deleteStaleCopy ();
}
//-----------------------------------------------------------------------------
