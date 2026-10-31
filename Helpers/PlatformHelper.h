#pragma once

#include <cstdint>

//-----------------------------------------------------------------------------

// Things JUCE has no API for, rolled by hand per platform, implemented in
// PlatformHelper_Win.cpp / PlatformHelper_Mac.mm. Declarations only:
// consumers never pull in <windows.h> & friends (whose SID type and ERROR
// macro clash with the project's symbols).

void setWindowProperties ( void* windowHandle, unsigned int titleColor );

// Free physical RAM right now; 0 if the query failed
[[ nodiscard ]] int64_t availableMemoryBytes ();

// Startup only: launches outside Explorer's chain (debugger, scripts, shells)
// carry no Windows foreground right and the window opens behind everything;
// this claims it back. No-op elsewhere
void bringWindowToForeground ( void* windowHandle );

// The OS-configured digit grouping where the C locale environment is blind
// (macOS GUI apps get no LANG); the defaults double as the American fallback
struct NumberGrouping
{
	bool	valid = false;			// The platform answered (macOS only)
	wchar_t	separator = L',';
	int		groupSize = 3;			// 0 = the user disabled grouping
	int		secondaryGroupSize = 0;	// 0 = groupSize repeats (India: 3, then 2s)
};

[[ nodiscard ]] NumberGrouping userNumberGrouping ();

// Authenticode self-check, Windows only: the signature seals the exe and its
// appended pak in one hash, so corrupted means shipped bytes changed. Dev
// builds and the other platforms report notSigned (macOS enforces its own
// signature at launch)
enum class SignatureState { notSigned, valid, corrupted };

[[ nodiscard ]] SignatureState verifyExecutableSignature ();

// Windows only: an enabled inbound block rule pinned to this exe, the lasting
// mark of a cancelled firewall prompt. No admin rights needed to ask. Other
// platforms (and a failed query) report false
[[ nodiscard ]] bool firewallBlocksThisApp ();
//-----------------------------------------------------------------------------
