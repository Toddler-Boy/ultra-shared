#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// Transparent full-window layer drawing a ring around the keyboard-focused
// component while the user navigates by keyboard: a key press arms it, a
// mouse click hides it. Components with a focus visual of their own are
// skipped. The owner feeds it the focus changes
class GUI_FocusRing final : public juce::Component
{
public:
	GUI_FocusRing ();
	~GUI_FocusRing () override;

	// juce::Component
	void paint ( juce::Graphics& g ) override;

	// this
	void focusChanged ( juce::Component* focused );
	void keyboardUsed ()	{	keyboardNav = true;	}

private:
	// juce::MouseListener, registered globally: a click hides the ring
	void mouseDown ( const juce::MouseEvent& e ) override;

	// Follows the focused component through moves, scrolls and relayouts
	class Watcher final : public juce::ComponentMovementWatcher
	{
	public:
		Watcher ( juce::Component& target, GUI_FocusRing& _owner )
			: juce::ComponentMovementWatcher ( &target )
			, owner ( _owner )
		{
		}

		void componentMovedOrResized ( bool, bool ) override	{	owner.update ();	}
		void componentPeerChanged () override					{	owner.update ();	}
		void componentVisibilityChanged () override				{	owner.update ();	}

	private:
		GUI_FocusRing&	owner;
	};

	std::unique_ptr<Watcher>	watcher;
	juce::Component::SafePointer<juce::Component>	target;

	juce::Rectangle<int>	ring;
	bool					keyboardNav = false;

	void update ();

	[[ nodiscard ]] static bool hasOwnFocusVisual ( const juce::Component& c );

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_FocusRing )
};
//-----------------------------------------------------------------------------
