#pragma once

//-----------------------------------------------------------------------------

namespace buildinfo
{
	// Development builds only: gates dev features like the factory-data
	// watcher and the curation menus. Constant-folds away in user builds
	[[ nodiscard ]] constexpr bool isDeveloperMode ()
	{
#if ULTRA_DEVELOPMENT
		return true;
#else
		return false;
#endif
	}
}
//-----------------------------------------------------------------------------
