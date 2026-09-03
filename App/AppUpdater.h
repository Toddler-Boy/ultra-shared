#pragma once

#include <JuceHeader.h>

#include <functional>

#include "Config/Preferences.h"
#include "Config/Settings.h"

//-----------------------------------------------------------------------------

// Checks the website for a newer app release (this platform's manifest: version,
// url, sha256) and can download, verify and swap that release in

class AppUpdater final
{
public:
	// manifestBaseURL holds the per-platform latest-<os>.json manifests
	explicit AppUpdater ( const juce::String& manifestBaseURL );

	// unknown = never checked or the last check failed, current = running the
	// published version, outdated = a newer release is out, the rest = install phases
	enum class State { unknown, current, outdated, updating, downloadFailed, corrupted, replaceFailed };

	struct Release
	{
		std::string		version;
		std::string		url;
		std::string		sha256;
		std::string		notes;
	};

	// Async, start from the message thread. check () honors the frequency
	// preference and the throttle, checkNow () always fetches
	void check ();
	void checkNow ();

	// The swap-in-place exists for Windows and macOS
	#if JUCE_WINDOWS || JUCE_MAC
		static constexpr bool	canInstall = true;
	#else
		static constexpr bool	canInstall = false;
	#endif

	// Downloads latest (), verifies its sha256, swaps it in beside the running
	// program and asks for the quit via onInstalled; the relaunch follows the exit
	void install ();

	// A known update waits to be installed (or retried)
	[[ nodiscard ]] bool updatePending () const;

	// All fire on the message thread: onStateChanged after every completed
	// (non-skipped) check and every install step, onProgress with 0..1
	std::function<void ( State )>	onStateChanged;
	std::function<void ( float )>	onProgress;
	std::function<void ()>			onInstalled;

	// Primed from the stored result of the last successful check
	[[ nodiscard ]] State state () const	{	return currentState;	}

	// The best-known release; only a successful check fills url/sha256/notes
	[[ nodiscard ]] const Release& latest () const	{	return latestRelease;	}

	// For main (), once JUCE has shut down and released the instance lock
	static void relaunchIfInstalled ();

private:
	static bool isNewer ( const std::string& available, const std::string& installed );
	static bool stageProgram ( const juce::MemoryBlock& data, const juce::File& fresh );
	static bool replaceProgram ( const juce::MemoryBlock& data );

	[[ nodiscard ]] bool isStale () const;

	void fetch ( bool installAfter = false );
	void setState ( State state );

	juce::String	manifestURL;

	State		currentState = State::unknown;
	Release		latestRelease;

	static inline bool	installed = false;

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Settings>		settings;
	gin::DownloadManager	downloader { 5 * 1000, 5 * 1000 };
};
//-----------------------------------------------------------------------------
