#include "C64u_Scanner.h"
#include "NetworkHardwareChecker.h"

#include "Config/Settings.h"

//-----------------------------------------------------------------------------

C64uScanner::C64uScanner () : juce::Thread ( "C64uScanner" )
{
}
//-----------------------------------------------------------------------------

C64uScanner::~C64uScanner ()
{
	stopThread ( -1 );
}
//-----------------------------------------------------------------------------

void C64uScanner::scan ( ScannerCallback _callback, juce::String& _lastIP )
{
	callback = std::move ( _callback );
	lastIP = std::move ( _lastIP );

	startThread ( juce::Thread::Priority::low );
}
//-----------------------------------------------------------------------------

juce::String C64uScanner::localAddressFor ( const juce::String& peerIP )
{
	const juce::IPAddress	peer ( peerIP );

	for ( const auto& addr : juce::IPAddress::getAllAddresses () )
	{
		if ( addr.isIPv6 )
			continue;

		if ( addr.address[ 0 ] == peer.address[ 0 ] && addr.address[ 1 ] == peer.address[ 1 ] && addr.address[ 2 ] == peer.address[ 2 ] )
			return addr.toString ();
	}

	return {};
}
//-----------------------------------------------------------------------------

void C64uScanner::run ()
{
	Z_LOG ( "C64uScanner started" );

	// Fresh per sweep: the password may have changed, and a stale refusal
	// must not outlive the sweep that saw it
	{
		const juce::SharedResourcePointer<Settings>	settings;

		request = "GET /v1/info HTTP/1.1\r\n\r\n";
		if ( const auto password = settings->get<juce::String> ( "network/password" ); password.isNotEmpty () )
			request = "GET /v1/info HTTP/1.1\r\nX-Password: " + password + "\r\n\r\n";

		const juce::ScopedLock	sl ( lockedLock );
		lockedIP.clear ();
	}

	NetworkHardwareChecker	hardware;

	auto	privateNetworks = juce::IPAddress::getAllAddresses ();

	// Remove all non-private network adapters
	privateNetworks.removeIf ( [ &hardware ] ( const juce::IPAddress& addr )
	{
		const auto&	b = addr.address;

		const auto	isPrivate = ( b[ 0 ] == 10 ) ||
								( b[ 0 ] == 192 && b[ 1 ] == 168 ) ||
								( b[ 0 ] == 172 && ( b[ 1 ] >= 16 && b[ 1 ] <= 31 ) );

		// It's a public IP, remove it
		if ( ! isPrivate )
			return true;

		// Then drop adapters that are down; wireless ones receive just fine
		return ! hardware.isActive ( addr );
	} );

	// Sort descending, so that 192.168.x.x is scanned first, as it's the most common private network range
	std::ranges::sort ( privateNetworks, [] ( const juce::IPAddress& a, const juce::IPAddress& b )
	{
		return a.address[ 0 ] > b.address[ 0 ];
	} );

	if ( privateNetworks.isEmpty ()  )
	{
		Z_ERR ( "C64uScanner failed to find any private network adapters" );

		if ( callback )
			callback ( {}, false );

		return;
	}

	// Create complete list of IPs to scan
	juce::StringArray	ipsToScan;

	for ( const auto& ip : privateNetworks )
	{
		const auto	base = ip.toString ().upToLastOccurrenceOf ( ".", true, false );

		// Let's try IP 64 first, as large portion of users will probably pick that one...
		constexpr auto	firstIndex = 64;
		for ( auto i = 0; i < 256; ++i )
		{
			const auto	scanIp = ( i + firstIndex ) & 255;

			if ( scanIp == 0 || scanIp == 255 )
				continue;

			ipsToScan.add ( base + juce::String ( scanIp ) );
		}

		// Remove local IP from the list, as we don't want to scan it
		ipsToScan.removeString ( ip.toString () );
	}

	if ( lastIP.isNotEmpty () )
	{
		ipsToScan.removeString ( lastIP );
		ipsToScan.insert ( 0, lastIP );
	}

	juce::ThreadPool	pool ( 20, juce::Thread::osDefaultStackSize, juce::Thread::Priority::low );

	Z_LOG ( "C64uScanner loop started" );

	std::atomic<bool>	found = false;

	for ( const auto& targetIP : ipsToScan )
	{
		pool.addJob ( [ &pool, &found, targetIP, this ]
		{
			juce::StreamingSocket   socket;

			if ( socket.connect ( targetIP, 80, 200 ) )
			{
				Z_LOG ( "Scanned device on " + targetIP );

				if ( auto hostName = isActualC64u ( socket ); hostName.isNotEmpty () )
				{
					found = true;

					if ( callback )
						callback ( targetIP + " (" + hostName + ")", false );

					pool.removeAllJobs ( false, 300 );
				}
			}
		} );
	}

	while ( pool.getNumJobs () > 0 )
	{
		if ( threadShouldExit () )
		{
			pool.removeAllJobs ( false, 300 );
			return;
		}

		wait ( 50 );
	}

	// A fruitless sweep reports too, so the caller can schedule a retry; a
	// refusing C64u only counts when nothing answered properly
	if ( ! found && callback )
	{
		const juce::ScopedLock	sl ( lockedLock );
		callback ( lockedIP, lockedIP.isNotEmpty () );
	}
}
//-----------------------------------------------------------------------------

juce::String C64uScanner::isActualC64u ( juce::StreamingSocket& socket )
{
	socket.write ( request.toRawUTF8 (), request.length () );

	// Sliced reads with a hard budget: a stranger keeping the connection open
	// would park a blocking read (and app shutdown behind it) on ITS timeout;
	// the C64u answers and closes within the first slices
	char	buffer[ 1024 ];
	auto	numBytes = 0;

	for ( auto budget = 10; --budget >= 0 && numBytes < int ( sizeof ( buffer ) - 1 ) && ! threadShouldExit (); )
	{
		const auto	ready = socket.waitUntilReady ( true, 50 );
		if ( ready < 0 )
			break;

		if ( ready == 0 )
			continue;

		const auto	bytesRead = socket.read ( buffer + numBytes, int ( sizeof ( buffer ) - 1 ) - numBytes, false );
		if ( bytesRead <= 0 )
			break;

		numBytes += bytesRead;
	}

	if ( numBytes > 0 )
	{
		buffer[ numBytes ] = 0;
		const juce::String    response ( buffer );

		if ( response.contains ( "HTTP/1.1 200 OK\r\n" ) && response.containsIgnoreCase ( "Ultimate" ) )
		{
			const auto	bodyStart = response.indexOf ( "\r\n\r\n" );
			const auto	json = juce::JSON::parse ( response.substring ( bodyStart + 4 ) );

			Z_INFO ( "C64uScanner found a C64u at " + socket.getHostName () + " with hostname: " + json[ "hostname" ].toString () );

			return json[ "hostname" ].toString ();
		}

		// The API answers 403 to a missing or wrong password
		if ( response.startsWith ( "HTTP/1.1 403" ) )
		{
			Z_INFO ( "C64uScanner got a password refusal from " + socket.getHostName () );

			const juce::ScopedLock	sl ( lockedLock );
			lockedIP = socket.getHostName ();
		}
	}
	else
	{
		Z_ERR ( "C64uScanner failed to read from " + socket.getHostName () );
	}
	return {};
}
//-----------------------------------------------------------------------------
