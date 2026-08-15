#include "AsyncNetwork.h"

//-----------------------------------------------------------------------------

AsyncNetwork::AsyncNetwork ()
	: juce::Thread ( "NetworkUploader" )
{
	startThread ( Priority::low );
}
//-----------------------------------------------------------------------------

AsyncNetwork::~AsyncNetwork ()
{
	signalThreadShouldExit ();
	wakeUp.signal ();

	{
		const juce::ScopedLock	sl ( streamLock );

		if ( activeStream )
			activeStream->cancel ();
	}

	waitForThreadToExit ( 5000 );
}
//-----------------------------------------------------------------------------

void AsyncNetwork::setBaseAddress ( const juce::String& url )
{
	baseAddress = juce::URL ( url );
}
//-----------------------------------------------------------------------------

void AsyncNetwork::setC64uPassword ( const juce::String& password )
{
	// A set-but-missing password makes the C64u go silent; the log is the only hint
	if ( ! passwordLogged || password != c64uPassword )
	{
		if ( password.isEmpty () )
			Z_INFO ( "no password set" );
		else
			Z_INFO ( "using password for http" );

		passwordLogged = true;
	}

	c64uPassword = password;
}
//-----------------------------------------------------------------------------

void AsyncNetwork::get ( const juce::String& endpoint, const juce::StringArray& params, NetworkCallback callback )
{
	enqueue ( endpoint, "GET", {}, std::move ( callback ), params );
}
//-----------------------------------------------------------------------------

void AsyncNetwork::post ( const juce::String& endpoint, const juce::MemoryBlock& data, NetworkCallback callback, const juce::StringArray& params )
{
	enqueue ( endpoint, "POST", data, std::move ( callback ), params );
}
//-----------------------------------------------------------------------------

void AsyncNetwork::put ( const juce::String& endpoint, const juce::StringArray& params, NetworkCallback callback )
{
	enqueue ( endpoint, "PUT", {}, std::move ( callback ), params );
}
//-----------------------------------------------------------------------------

void AsyncNetwork::enqueue ( const juce::String& endpoint, const juce::String& method, const juce::MemoryBlock& data, NetworkCallback cb, const juce::StringArray& params )
{
	auto	requestUrl = baseAddress.getChildURL ( endpoint );

	if ( ! data.isEmpty () )
		requestUrl = requestUrl.withPOSTData ( data );

	for ( auto i = 0; i < params.size (); i += 2 )
		requestUrl = requestUrl.withParameter ( params[ i ], params[ i + 1 ] );

	const juce::ScopedLock	sl ( queueLock );
	queue.add ( { requestUrl, method, std::move ( cb ) } );
	wakeUp.signal ();
}
//-----------------------------------------------------------------------------

void AsyncNetwork::run ()
{
	while ( ! threadShouldExit () )
	{
		queueLock.enter ();
		if ( queue.isEmpty () )
		{
			queueLock.exit ();
			wakeUp.wait ();
			continue;
		}

		auto	req = queue.removeAndReturn ( 0 );
		queueLock.exit ();

		const auto	headers = c64uPassword.isNotEmpty () ? juce::String ( "X-Password: " ) + c64uPassword : juce::String ();

		{
			const juce::ScopedLock	sl ( streamLock );
			activeStream = std::make_unique<juce::WebInputStream> ( req.url, req.method == "POST" );
		}

		activeStream->withExtraHeaders ( headers )
					 .withCustomRequestCommand ( req.method )
					 .withConnectionTimeout ( 200 )
					 .withNumRedirectsToFollow ( 0 );

		if ( activeStream->connect ( nullptr ) )
		{
			req.statusCode = activeStream->getStatusCode ();

			if ( req.callback )
			{
				const auto	jsonString = activeStream->readEntireStreamAsString ();
				const auto	json = juce::JSON::parse ( jsonString );

				req.callback ( json, req.statusCode );
			}
		}
		else
		{
			if ( req.callback )
				req.callback ( {}, 0 );
		}

		const juce::ScopedLock	sl ( streamLock );
		activeStream.reset ();
	}
}
//-----------------------------------------------------------------------------
