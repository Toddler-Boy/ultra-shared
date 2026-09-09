#include "ultra-shared/Helpers/PlatformHelper.h"

#if defined (__linux__)

#include <cstdio>

// No custom window chrome on Linux

void setWindowProperties ( void*, unsigned int )
{
}
//-----------------------------------------------------------------------------

int64_t availableMemoryBytes ()
{
	// MemAvailable counts reclaimable cache, MemFree does not
	if ( auto file = std::fopen ( "/proc/meminfo", "r" ) )
	{
		char		line[ 128 ];
		long long	kb = 0;

		while ( std::fgets ( line, sizeof ( line ), file ) )
			if ( std::sscanf ( line, "MemAvailable: %lld kB", &kb ) == 1 )
				break;

		std::fclose ( file );
		return int64_t ( kb ) * 1024;
	}

	return 0;
}
//-----------------------------------------------------------------------------

NumberGrouping userNumberGrouping ()
{
	return {};
}
//-----------------------------------------------------------------------------

void bringWindowToForeground ( void* )
{
}
//-----------------------------------------------------------------------------

SignatureState verifyExecutableSignature ()
{
	return SignatureState::notSigned;
}
//-----------------------------------------------------------------------------

bool firewallBlocksThisApp ()
{
	return false;
}
//-----------------------------------------------------------------------------

#endif
