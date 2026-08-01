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
//-----------------------------------------------------------------------------
