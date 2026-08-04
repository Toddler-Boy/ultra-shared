#include "ultra-shared/Helpers/PlatformHelper.h"

#if defined (_WIN32) || defined (_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Both apps render the GL CRT stack: ask switchable-graphics machines for
// the discrete GPU

extern "C" {
	// NVIDIA: Enables high-performance mode
	_declspec( dllexport ) DWORD NvOptimusEnablement = 0x00000001;

	// AMD: Enables high-performance mode
	_declspec( dllexport ) int AmdPowerXpressRequestHighPerformance = 1;
}
//-----------------------------------------------------------------------------

void setWindowProperties ( void* windowHandle, unsigned int titleColor )
{
	if ( auto hDwmApi = LoadLibrary ("dwmapi.dll"); hDwmApi )
	{
		typedef HRESULT ( WINAPI* PFNSETWINDOWATTRIBUTE )( HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute );

		if ( auto pfnSetWindowAttribute = reinterpret_cast<PFNSETWINDOWATTRIBUTE>( GetProcAddress ( hDwmApi, "DwmSetWindowAttribute" ) ); pfnSetWindowAttribute )
		{
			enum : DWORD
			{
				DWMWA_USE_IMMERSIVE_DARK_MODE = 20,
				DWMWA_BORDER_COLOR = 34,
				DWMWA_CAPTION_COLOR,

				DWMWA_COLOR_NONE = 0xFFFFFFFE,
				DWMWA_COLOR_DEFAULT,
			};

			BOOL	useDarkMode = TRUE;
			pfnSetWindowAttribute ( (HWND)windowHandle, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDarkMode, sizeof ( useDarkMode ) );

			DWORD	color = ( ( titleColor >> 16 ) & 0x0000FF ) | ( titleColor & 0x00FF00 ) | ( ( titleColor << 16 ) & 0xFF0000 );
 			pfnSetWindowAttribute ( (HWND)windowHandle, DWMWA_CAPTION_COLOR, &color, sizeof ( color ) );
		}
		FreeLibrary ( hDwmApi );
	}
}
//-----------------------------------------------------------------------------

int64_t availableMemoryBytes ()
{
	MEMORYSTATUSEX	status = {};
	status.dwLength = sizeof ( status );

	if ( GlobalMemoryStatusEx ( &status ) )
		return int64_t ( status.ullAvailPhys );

	return 0;
}
//-----------------------------------------------------------------------------

NumberGrouping userNumberGrouping ()
{
	// std::locale ( "" ) reads the region settings here, no help needed
	return {};
}
//-----------------------------------------------------------------------------

void bringWindowToForeground ( void* windowHandle )
{
	const auto	hwnd = (HWND) windowHandle;

	// SetForegroundWindow only obeys the thread that owns the foreground;
	// borrow its input state for the one call
	const auto	fg = GetForegroundWindow ();
	const auto	fgThread = fg ? GetWindowThreadProcessId ( fg, nullptr ) : 0;
	const auto	ourThread = GetCurrentThreadId ();
	const auto	attached = fgThread && fgThread != ourThread && AttachThreadInput ( fgThread, ourThread, TRUE );

	SetForegroundWindow ( hwnd );
	BringWindowToTop ( hwnd );

	if ( attached )
		AttachThreadInput ( fgThread, ourThread, FALSE );
}
//-----------------------------------------------------------------------------

#endif
