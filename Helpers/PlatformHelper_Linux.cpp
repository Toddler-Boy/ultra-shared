#include "ultra-shared/Helpers/PlatformHelper.h"

#if defined (__linux__)

// No custom window chrome on Linux, and no memory probe yet: stubs keep the
// header honest on every platform

void setWindowProperties ( void*, unsigned int )
{
}
//-----------------------------------------------------------------------------

int64_t availableMemoryBytes ()
{
	return 0;
}
//-----------------------------------------------------------------------------

NumberGrouping userNumberGrouping ()
{
	return {};
}
//-----------------------------------------------------------------------------

#endif
