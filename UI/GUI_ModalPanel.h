#pragma once

#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_SVG_Button.h"

//-----------------------------------------------------------------------------

// The full-window modal overlay the dialogs share (About, Shortcuts): the
// blurred snapshot of the window behind it, a centered drop-shadowed panel
// ("body", sized by UI/layouts/<name>.json) the subclass fills, and a close
// button plus Escape, both sending closeVerb on the message bus. The host
// shows/hides the panel on its show/close verbs.

class GUI_ModalPanel : public juce::Component
{
public:
	GUI_ModalPanel ( const juce::String& name, const char* closeVerb );

	// juce::Component
	void resized () override;
	void paint ( juce::Graphics& g ) override;
	bool keyPressed ( const juce::KeyPress& key ) override;

	// this
	// The component overload snapshots comp; pass a finished image when the
	// snapshot needs content the software renderer cannot paint
	void setBackground ( juce::Component* comp );
	void setBackground ( juce::Image snapshot );

protected:
	// The visible panel: children clip to it, the overlay around it stays clear
	juce::Component		body { "body" };
	GUI_SVG_Button		close { "close", { "about/close" } };

private:
	void requestClose () const;

	const char*				closeVerb;
	juce::String			layoutFile;

	melatonin::DropShadow	shadow { 12.0 };
	gin::LayoutSupport		layout { *this };

	juce::Image				background;
	juce::Path				shadowPath;
	juce::Rectangle<int>	bodyBounds;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ModalPanel )
};
//-----------------------------------------------------------------------------
