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
//-----------------------------------------------------------------------------
