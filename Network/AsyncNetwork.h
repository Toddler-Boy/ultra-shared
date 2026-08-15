#pragma once
#include <JuceHeader.h>

//-----------------------------------------------------------------------------

class AsyncNetwork : private juce::Thread
{
public:
	using NetworkCallback = std::function<void ( const juce::var&, const int )>;

	AsyncNetwork ();
	~AsyncNetwork () override;

	void setBaseAddress ( const juce::String& url );
	void setC64uPassword ( const juce::String& password );

	void get ( const juce::String& endpoint, const juce::StringArray& params = {}, NetworkCallback callback = nullptr );
	void post ( const juce::String& endpoint, const juce::MemoryBlock& data, NetworkCallback callback = nullptr, const juce::StringArray& params = {} );
	void put ( const juce::String& endpoint, const juce::StringArray& params = {}, NetworkCallback callback = nullptr );

private:
	struct RequestData
	{
		juce::URL			url;
		juce::String		method;
		NetworkCallback		callback;
		int					statusCode;
	};

	void run () override;
	void enqueue ( const juce::String& endpoint, const juce::String& method, const juce::MemoryBlock& data, NetworkCallback cb, const juce::StringArray& params );

	juce::URL			baseAddress;
	juce::String		c64uPassword;
	bool				passwordLogged = false;

	juce::CriticalSection		queueLock;
	juce::Array<RequestData>	queue;
	juce::WaitableEvent			wakeUp;

	// The in-flight request, held so the destructor can abort a connect that
	// blocks on a silent device
	juce::CriticalSection					streamLock;
	std::unique_ptr<juce::WebInputStream>	activeStream;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( AsyncNetwork )
};
//-----------------------------------------------------------------------------
