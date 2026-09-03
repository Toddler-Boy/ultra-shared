#include <JuceHeader.h>

#include "ultra-shared/App/AppUpdater.h"

#if JUCE_MAC
	#include <unistd.h>
#endif

//-----------------------------------------------------------------------------

#if JUCE_WINDOWS
	constexpr auto	manifestName = "/latest-windows.json";
#elif JUCE_MAC
	constexpr auto	manifestName = "/latest-macos.json";
#else
	constexpr auto	manifestName = "/latest-linux.json";
#endif

//-----------------------------------------------------------------------------

// "85.0.2" -> { 85, 0, 2 }; missing or malformed parts read as 0
static std::array<int, 3> parseVersion ( const juce::String& text )
{
	const auto	parts = juce::StringArray::fromTokens ( text, ".", {} );

	std::array<int, 3>	v {};
	for ( auto i = 0; i < int ( v.size () ); ++i )
		v[ size_t ( i ) ] = parts[ i ].getIntValue ();

	return v;
}
//-----------------------------------------------------------------------------

// What an update replaces: the exe on Windows, the whole bundle on macOS
static juce::File installedProgram ()
{
	#if JUCE_MAC
		return juce::File::getSpecialLocation ( juce::File::currentApplicationFile );
	#else
		return juce::File::getSpecialLocation ( juce::File::currentExecutableFile );
	#endif
}
//-----------------------------------------------------------------------------

// The running program's name plus a suffix, in its own folder
static juce::File programSibling ( const char* suffix )
{
	const auto	program = installedProgram ();

	return program.getSiblingFile ( program.getFileName () + suffix );
}
//-----------------------------------------------------------------------------

bool AppUpdater::isNewer ( const std::string& available, const std::string& installed )
{
	return parseVersion ( available ) > parseVersion ( installed );
}
//-----------------------------------------------------------------------------

AppUpdater::AppUpdater ( const juce::String& manifestBaseURL )
	: manifestURL ( manifestBaseURL + manifestName )
{
	// The program a swap renamed away can only go once that process has
	// exited; a failed delete simply waits for the next start
	if constexpr ( canInstall )
		programSibling ( ".old" ).deleteRecursively ();

	// The stored result of the last successful check bridges throttle-skipped
	// starts; an "up to date" older than six months no longer counts, a known
	// update stays known regardless of age
	latestRelease.version = settings->get<std::string> ( "update/last-known-version" );

	if ( latestRelease.version.empty () )
		return;

	if ( isNewer ( latestRelease.version, ProjectInfo::versionString ) )
		currentState = State::outdated;
	else if ( ! isStale () )
		currentState = State::current;
}
//-----------------------------------------------------------------------------

// True when the last successful check is more than six months old (or its
// stamp unreadable)
bool AppUpdater::isStale () const
{
	const auto	lastCheck = juce::Time::fromISO8601 ( settings->get<juce::String> ( "update/last-check" ) );
	const auto	ageMin = ( juce::Time::getCurrentTime () - lastCheck ).inMinutes ();

	return ageMin < 0 || ageMin >= 6 * 30 * 24 * 60;
}
//-----------------------------------------------------------------------------

void AppUpdater::setState ( const State state )
{
	currentState = state;

	if ( onStateChanged )
		onStateChanged ( currentState );
}
//-----------------------------------------------------------------------------

bool AppUpdater::updatePending () const
{
	return currentState == State::outdated || currentState == State::downloadFailed
		|| currentState == State::corrupted || currentState == State::replaceFailed;
}
//-----------------------------------------------------------------------------

void AppUpdater::check ()
{
	if ( ! preferences->get<bool> ( "update/check" ) )
	{
		Z_INFO ( "App update check disabled" );
		return;
	}

	// An unknown check-frequency value reads as the daily default
	const auto	frequency = preferences->get<juce::String> ( "update/check-frequency" );
	const auto	intervalMin = ( frequency == "weekly" ? 7 : frequency == "monthly" ? 30 : 1 ) * 24 * 60;

	// Any successful check stamps the clock, failures retry on the next
	// start; the skip also needs a stored result to bridge with
	const auto	lastCheck = juce::Time::fromISO8601 ( settings->get<juce::String> ( "update/last-check" ) );
	const auto	ageMin = int ( ( juce::Time::getCurrentTime () - lastCheck ).inMinutes () );

	if ( ageMin >= 0 && ageMin < intervalMin && ! latestRelease.version.empty () )
	{
		Z_INFO ( "App update check skipped, last checked " << ageMin / 60 << "h " << ageMin % 60 << "m ago" );
		return;
	}

	fetch ();
}
//-----------------------------------------------------------------------------

void AppUpdater::checkNow ()
{
	fetch ();
}
//-----------------------------------------------------------------------------

void AppUpdater::fetch ( const bool installAfter )
{
	downloader.startAsyncDownload ( juce::URL ( manifestURL ), [ this, installAfter ] ( gin::DownloadManager::DownloadResult res )
	{
		// A failed check downgrades "current" to "unknown", but never hides a
		// known update; on the way to an install it counts as a failed download
		const auto	degrade = [ this, installAfter ]
		{
			setState ( installAfter ? State::downloadFailed
					 : currentState == State::outdated ? State::outdated : State::unknown );
		};

		if ( ! res.ok )
		{
			Z_ERR ( "Couldn't download the app update manifest (" << manifestURL << ") - HTTP/" << res.httpCode );
			degrade ();
			return;
		}

		const auto	parsed = juce::JSON::parse ( juce::String ( (const char*) res.data.getData (), res.data.getSize () ) );
		if ( ! parsed.isObject () )
		{
			Z_ERR ( "App update manifest is not valid JSON (" << manifestURL << ")" );
			degrade ();
			return;
		}

		Release	release;
		release.version = parsed.getProperty ( "version", "" ).toString ().toStdString ();
		release.url = parsed.getProperty ( "url", "" ).toString ().toStdString ();
		release.sha256 = parsed.getProperty ( "sha256", "" ).toString ().toStdString ();
		release.notes = parsed.getProperty ( "notes", "" ).toString ().toStdString ();

		if ( release.version.empty () || release.url.empty () || release.sha256.empty () )
		{
			Z_ERR ( "App update manifest misses required fields (" << manifestURL << ")" );
			degrade ();
			return;
		}

		latestRelease = release;

		settings->set ( "update/last-check", juce::Time::getCurrentTime ().toISO8601 ( true ).toStdString () );
		settings->set ( "update/last-known-version", latestRelease.version );

		if ( ! isNewer ( latestRelease.version, ProjectInfo::versionString ) )
		{
			Z_INFO ( "App is up to date (running " << ProjectInfo::versionString << ", published " << latestRelease.version << ")" );
			setState ( State::current );
			return;
		}

		Z_INFO ( "App update available: " << latestRelease.version << " (running " << ProjectInfo::versionString << ")" );
		setState ( State::outdated );

		if ( installAfter )
			install ();
	} );
}
//-----------------------------------------------------------------------------

void AppUpdater::install ()
{
	if ( currentState == State::updating )
		return;

	// A stored check result carries no download url, a live one fills it in
	if ( latestRelease.url.empty () )
	{
		fetch ( true );
		return;
	}

	setState ( State::updating );

	downloader.startAsyncDownload ( juce::URL ( latestRelease.url ), [ this ] ( gin::DownloadManager::DownloadResult res )
	{
		if ( ! res.ok )
		{
			Z_ERR ( "Couldn't download the app update (" << latestRelease.url << ") - HTTP/" << res.httpCode );
			setState ( State::downloadFailed );
			return;
		}

		if ( ! juce::SHA256 ( res.data ).toHexString ().equalsIgnoreCase ( juce::String ( latestRelease.sha256 ) ) )
		{
			Z_ERR ( "The downloaded app update doesn't match the manifest's sha256, discarded" );
			setState ( State::corrupted );
			return;
		}

		if ( ! replaceProgram ( res.data ) )
		{
			setState ( State::replaceFailed );
			return;
		}

		Z_INFO ( "App update " << latestRelease.version << " installed, relaunching" );
		installed = true;

		if ( onInstalled )
			onInstalled ();
	},
	[ this ] ( const juce::int64 current, const juce::int64 total, juce::int64 )
	{
		if ( onProgress )
			onProgress ( total > 0 ? float ( current ) / float ( total ) : 0.0f );
	} );
}
//-----------------------------------------------------------------------------

// Puts the downloaded program at fresh, beside the running one. Windows: the
// bare exe. macOS: mount the dmg, ditto keeps the bundle's signature intact
bool AppUpdater::stageProgram ( const juce::MemoryBlock& data, const juce::File& fresh )
{
	#if JUCE_WINDOWS
		if ( fresh.replaceWithData ( data.getData (), data.getSize () ) )
			return true;

		Z_ERR ( "Couldn't write the app update next to the exe: " << fresh.getFullPathName () );
		return false;
	#elif JUCE_MAC
		// Gatekeeper runs a quarantined bundle from a throwaway copy
		if ( fresh.getFullPathName ().contains ( "/AppTranslocation/" ) )
		{
			Z_ERR ( "The app runs translocated, there is no installed bundle to replace" );
			return false;
		}

		const auto	temp = juce::File::getSpecialLocation ( juce::File::tempDirectory );
		const auto	dmg = temp.getChildFile ( juce::String ( ProjectInfo::projectName ) + "-update.dmg" );
		const auto	mount = temp.getChildFile ( juce::String ( ProjectInfo::projectName ) + "-update" );

		if ( ! dmg.replaceWithData ( data.getData (), data.getSize () ) )
		{
			Z_ERR ( "Couldn't write the app update dmg: " << dmg.getFullPathName () );
			return false;
		}

		const auto	run = [] ( const juce::StringArray& command, juce::String& output )
		{
			juce::ChildProcess	process;

			if ( ! process.start ( command ) )
				return false;

			output = process.readAllProcessOutput ();
			return process.getExitCode () == 0;
		};

		const auto	source = mount.getChildFile ( juce::String ( ProjectInfo::projectName ) + ".app" );

		const juce::StringArray	attach ( "/usr/bin/hdiutil", "attach", "-nobrowse", "-readonly", "-noverify", "-noautoopen", "-mountpoint", mount.getFullPathName (), dmg.getFullPathName () );
		const juce::StringArray	copy ( "/usr/bin/ditto", source.getFullPathName (), fresh.getFullPathName () );
		const juce::StringArray	detach ( "/usr/bin/hdiutil", "detach", mount.getFullPathName () );

		juce::String	output;

		mount.createDirectory ();

		if ( ! run ( attach, output ) )
		{
			Z_ERR ( "Couldn't mount the app update dmg: " << output );
			dmg.deleteFile ();
			return false;
		}

		const auto	copied = run ( copy, output );

		run ( detach, output );
		dmg.deleteFile ();

		if ( copied )
			return true;

		Z_ERR ( "Couldn't copy the app update out of its dmg: " << output );
		fresh.deleteRecursively ();
		return false;
	#else
		juce::ignoreUnused ( data, fresh );
		return false;
	#endif
}
//-----------------------------------------------------------------------------

// The new program lands beside the running one and the two swap by rename.
// Windows swaps the exe itself (a running exe can be renamed, not
// overwritten); macOS swaps the Contents folder, so the bundle folder keeps
// its inode and the Dock and Finder aliases still point at the same app
bool AppUpdater::replaceProgram ( const juce::MemoryBlock& data )
{
	const auto	program = installedProgram ();
	const auto	fresh = programSibling ( ".new" );
	const auto	old = programSibling ( ".old" );

	fresh.deleteRecursively ();
	old.deleteRecursively ();

	if ( ! stageProgram ( data, fresh ) )
		return false;

	#if JUCE_MAC
		const auto	current = program.getChildFile ( "Contents" );
		const auto	incoming = fresh.getChildFile ( "Contents" );
	#else
		const auto	current = program;
		const auto	incoming = fresh;
	#endif

	if ( ! current.moveFileTo ( old ) )
	{
		Z_ERR ( "Couldn't rename the running program: " << current.getFullPathName () );
		fresh.deleteRecursively ();
		return false;
	}

	if ( ! incoming.moveFileTo ( current ) )
	{
		Z_ERR ( "Couldn't move the app update into place: " << current.getFullPathName () );
		old.moveFileTo ( current );
		fresh.deleteRecursively ();
		return false;
	}

	// The emptied bundle shell (already gone on Windows)
	fresh.deleteRecursively ();

	return true;
}
//-----------------------------------------------------------------------------

void AppUpdater::relaunchIfInstalled ()
{
	if ( ! installed )
		return;

	#if JUCE_MAC
		// A second instance beside the exiting one gets its own Dock tile, so a
		// detached shell opens the bundle once this process is gone (30 s cap)
		const auto	script = "i=0; while kill -0 " + juce::String ( getpid () ) + " 2>/dev/null && [ $i -lt 300 ]; do sleep 0.1; i=$((i+1)); done; open '"
						  + installedProgram ().getFullPathName () + "'";

		const juce::StringArray	relaunch ( "/bin/sh", "-c", script );
		juce::ChildProcess		process;

		process.start ( relaunch, 0 );
	#else
		installedProgram ().startAsProcess ();
	#endif
}
//-----------------------------------------------------------------------------
