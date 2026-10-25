#include <JuceHeader.h>

#include "ultra-shared/Helpers/Broadcast.h"

//-----------------------------------------------------------------------------

void broadcast::send ( const juce::String& sender, const juce::String& event, const juce::var& body )
{
	juce::MessageManager::broadcastMessage ( sender + " " + event + " " + juce::JSON::toString ( body, true ) );
}
//-----------------------------------------------------------------------------

broadcast::Listener::Listener ( Handler handler_ )
	: handler ( std::move ( handler_ ) )
{
	juce::MessageManager::getInstance ()->registerBroadcastListener ( this );
}

broadcast::Listener::~Listener ()
{
	juce::MessageManager::getInstance ()->deregisterBroadcastListener ( this );
}

void broadcast::Listener::actionListenerCallback ( const juce::String& message )
{
	const auto	sender = message.upToFirstOccurrenceOf ( " ", false, false );
	const auto	rest = message.fromFirstOccurrenceOf ( " ", false, false );
	const auto	event = rest.upToFirstOccurrenceOf ( " ", false, false );

	// Only our own apps' lines have the shape; anything else on the machine's broadcast is not for us
	if ( ! sender.startsWith ( "ultra" ) || event.isEmpty () )
		return;

	handler ( sender, event, juce::JSON::parse ( rest.fromFirstOccurrenceOf ( " ", false, false ) ) );
}
//-----------------------------------------------------------------------------
