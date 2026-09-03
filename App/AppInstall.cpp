#include <JuceHeader.h>

#include "ultra-shared/App/AppInstall.h"

#include "Config/Settings.h"
#include "ultra-shared/App/AppUpdater.h"

#if JUCE_WINDOWS
	#include <ShlObj.h>
#endif

//-----------------------------------------------------------------------------

#if JUCE_WINDOWS

// FOLDERID_Programs, the user's Start Menu\Programs folder
static constexpr GUID	folderPrograms = { 0xA77F5D77, 0x2E2B, 0x44C3, { 0xA6, 0xA2, 0xAB, 0xA6, 0x01, 0x05, 0x4A, 0x51 } };

static juce::File startMenuFolder ()
{
	PWSTR	path = nullptr;

	if ( SHGetKnownFolderPath ( folderPrograms, 0, nullptr, &path ) != S_OK )
		return {};

	const juce::File	folder { juce::String ( path ) };
	CoTaskMemFree ( path );

	return folder;
}
//-----------------------------------------------------------------------------

static bool writeShortcut ( const juce::File& program )
{
	const auto	folder = startMenuFolder ();

	if ( folder == juce::File () )
	{
		Z_ERR ( "Couldn't resolve the Start menu folder" );
		return false;
	}

	const auto	link = folder.getChildFile ( juce::String ( ProjectInfo::projectName ) + ".lnk" );

	if ( program.createShortcut ( ProjectInfo::projectName, link ) )
		return true;

	Z_ERR ( "Couldn't write the Start menu shortcut: " << link.getFullPathName () );
	return false;
}

#endif
//-----------------------------------------------------------------------------

juce::File appinstall::programsFolderExe ()
{
	#if JUCE_WINDOWS
		return juce::File::getSpecialLocation ( juce::File::windowsLocalAppData )
				.getChildFile ( "Programs" )
				.getChildFile ( ProjectInfo::projectName )
				.getChildFile ( juce::String ( ProjectInfo::projectName ) + ".exe" );
	#else
		return {};
	#endif
}
//-----------------------------------------------------------------------------

bool appinstall::runsFromProgramsFolder ()
{
	if constexpr ( ! supported )
		return false;

	return juce::File::getSpecialLocation ( juce::File::currentExecutableFile ) == programsFolderExe ();
}
//-----------------------------------------------------------------------------

bool appinstall::createStartMenuShortcut ()
{
	#if JUCE_WINDOWS
		return writeShortcut ( juce::File::getSpecialLocation ( juce::File::currentExecutableFile ) );
	#else
		return false;
	#endif
}
//-----------------------------------------------------------------------------

bool appinstall::moveToProgramsFolder ()
{
	#if JUCE_WINDOWS
		const auto	src = juce::File::getSpecialLocation ( juce::File::currentExecutableFile );
		const auto	dst = programsFolderExe ();

		if ( src == dst )
			return true;

		if ( ! dst.getParentDirectory ().createDirectory () )
		{
			Z_ERR ( "Couldn't create the programs folder: " << dst.getParentDirectory ().getFullPathName () );
			return false;
		}

		// An older copy sitting there; a running one can't go and fails the move below
		dst.deleteFile ();

		// A running exe can be renamed but not copied away: the move is a rename
		// on one volume, otherwise a copy whose original goes on the next start
		if ( ! src.moveFileTo ( dst ) )
		{
			if ( ! src.copyFileTo ( dst ) )
			{
				Z_ERR ( "Couldn't move the program to " << dst.getFullPathName () );
				return false;
			}

			juce::SharedResourcePointer<Settings> ()->set ( "install/stale-copy", src.getFullPathName ().toStdString () );
		}

		writeShortcut ( dst );
		AppUpdater::relaunchAfterExit ( dst );

		Z_INFO ( "Program moved to " << dst.getFullPathName () << ", relaunching" );
		return true;
	#else
		return false;
	#endif
}
//-----------------------------------------------------------------------------

void appinstall::deleteStaleCopy ()
{
	if constexpr ( ! supported )
		return;

	juce::SharedResourcePointer<Settings>	settings;

	const auto	stale = settings->get<juce::String> ( "install/stale-copy" );

	if ( stale.isEmpty () )
		return;

	// Only ever our own program, never the running one
	const juce::File	file ( stale );

	if ( file != juce::File::getSpecialLocation ( juce::File::currentExecutableFile ) && file.existsAsFile () && ! file.deleteFile () )
	{
		Z_WARN ( "Couldn't remove the old program copy yet: " << stale );
		return;
	}

	settings->set ( "install/stale-copy", std::string () );
}
//-----------------------------------------------------------------------------
