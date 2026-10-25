#pragma once

#include <JuceHeader.h>

#include <functional>

//-----------------------------------------------------------------------------

// What one ultra app tells every other one running on this machine, through JUCE's broadcast to all its applications:
// "<sender> <event> <json body>", one line, fire and forget. Nobody listening = nothing happens; a listener drops what
// it does not know

namespace broadcast
{
	void send ( const juce::String& sender, const juce::String& event, const juce::var& body = {} );

	// Every broadcast of the ultra apps, on the message thread, for as long as the listener lives
	class Listener final : private juce::ActionListener
	{
	public:
		using Handler = std::function<void ( const juce::String& sender, const juce::String& event, const juce::var& body )>;

		explicit Listener ( Handler handler );
		~Listener () override;

	private:
		// juce::ActionListener
		void actionListenerCallback ( const juce::String& message ) override;

		Handler	handler;

		JUCE_DECLARE_NON_COPYABLE ( Listener )
	};
}
//-----------------------------------------------------------------------------
