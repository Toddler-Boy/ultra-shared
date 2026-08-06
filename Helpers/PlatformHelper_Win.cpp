#include "ultra-shared/Helpers/PlatformHelper.h"

#if defined (_WIN32) || defined (_WIN64)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <SoftPub.h>
#include <WinTrust.h>

// Keeps the dependency inside this file, no build-file entry in either app
#pragma comment ( lib, "wintrust" )

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

SignatureState verifyExecutableSignature ()
{
	wchar_t	path[ MAX_PATH ] = {};
	if ( ! GetModuleFileNameW ( nullptr, path, MAX_PATH ) )
		return SignatureState::notSigned;

	WINTRUST_FILE_INFO	fileInfo = {};
	fileInfo.cbStruct = sizeof ( fileInfo );
	fileInfo.pcwszFilePath = path;

	// No UI, no revocation lookups: offline machines must not stall on CRL
	// fetches, only the digest matters here
	WINTRUST_DATA	data = {};
	data.cbStruct = sizeof ( data );
	data.dwUIChoice = WTD_UI_NONE;
	data.fdwRevocationChecks = WTD_REVOKE_NONE;
	data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
	data.dwUnionChoice = WTD_CHOICE_FILE;
	data.pFile = &fileInfo;
	data.dwStateAction = WTD_STATEACTION_VERIFY;

	GUID		action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	const auto	status = WinVerifyTrust ( nullptr, &action, &data );
	const auto	detail = static_cast<LONG> ( GetLastError () );

	data.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust ( nullptr, &action, &data );

	if ( status == ERROR_SUCCESS )
		return SignatureState::valid;

	if ( status == TRUST_E_NOSIGNATURE )
	{
		// Truly unsigned (dev and local builds); any other detail means a
		// signature is present but unreadable, i.e. a damaged file
		if ( detail == TRUST_E_NOSIGNATURE || detail == TRUST_E_SUBJECT_FORM_UNKNOWN || detail == TRUST_E_PROVIDER_UNKNOWN )
			return SignatureState::notSigned;

		return SignatureState::corrupted;
	}

	// CERT_E chain and policy trouble (expired, untrusted root): the digest
	// matched to get that far, the file itself is whole
	if ( HRESULT_FACILITY ( status ) == FACILITY_CERT )
		return SignatureState::valid;

	// Everything else, from a wrong digest (TRUST_E_BAD_DIGEST) to a
	// signature blob that no longer parses (STATUS_INVALID_SIGNATURE), means
	// the file changed after signing
	return SignatureState::corrupted;
}
//-----------------------------------------------------------------------------

#endif
