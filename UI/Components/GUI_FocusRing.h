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

	// Ring rect = the target's visible area plus its margin. Margin = the
	// theme's focus-ring padding, scaled per side by the target's
	// "focusMargin" property (CSS shorthand of factors). Radius from its
	// "focusRadius" property: a number, or the target's own corner role name
	// (concentric: that corner plus the margin); the theme's focus-ring
	// corner otherwise
	struct Ring
	{
		juce::Rectangle<int>	rect;
		float					radius = 0.0f;

		bool operator== ( const Ring& ) const = default;
	};

	Ring	ring;
	bool	keyboardNav = false;

	void update ();

	[[ nodiscard ]] static juce::Rectangle<int> holeOf ( const Ring& r );
	[[ nodiscard ]] static bool hasOwnFocusVisual ( const juce::Component& c );

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_FocusRing )
};
//-----------------------------------------------------------------------------
