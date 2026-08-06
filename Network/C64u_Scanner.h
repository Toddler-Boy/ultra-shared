#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

class C64uScanner : private juce::Thread
{
public:
	// Empty ip = fruitless sweep; passwordRequired = a C64u answered but the
	// API refused (403), the ip is that machine
	using ScannerCallback = std::function<void ( const juce::String&, bool passwordRequired )>;

	C64uScanner ();
	~C64uScanner () override;

	void scan ( ScannerCallback _callback, juce::String& _lastIP );

	// Local adapter address on the same /24 as the given LAN peer, empty when
	// no adapter matches
	[[ nodiscard ]] static juce::String localAddressFor ( const juce::String& peerIP );

private:
	void run () override;

	juce::String isActualC64u ( juce::StreamingSocket& socket );

	ScannerCallback	callback;
	juce::String	lastIP;

	juce::String	request;

	juce::CriticalSection	lockedLock;
	juce::String			lockedIP;
};
//-----------------------------------------------------------------------------
