#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Helpers/MipMap.h"

#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_ScrollTextViewer.h"
#include "ultra-shared/UI/Components/GUI_SVG_Button.h"

//-----------------------------------------------------------------------------

class GUI_MipMap : public juce::Component
{
public:
	GUI_MipMap () = default;

	void paint ( juce::Graphics& g ) override
	{
		mipMap.draw ( g, getLocalBounds ().toFloat () );
	}

	MipMap	mipMap;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_MipMap )
};
//-----------------------------------------------------------------------------

// The About screen both apps share: a full-window overlay showing a centered,
// drop-shadowed panel ("body" in UI/layouts/about.json, which also sets the
// panel size). Shown/hidden by the host via "showAbout"/"closeAbout"

class GUI_About final : public juce::Component
{
public:
	GUI_About ();

	// juce::Component
	void resized () override;
	void paint ( juce::Graphics& g ) override;
	bool keyPressed ( const juce::KeyPress& key ) override;

	// this
	// The component overload snapshots comp; pass a finished image when the
	// snapshot needs content the software renderer cannot paint
	void setBackground ( juce::Component* comp );
	void setBackground ( juce::Image snapshot );
	void updateColors ();
	void loadContent ();

private:
	// The visible panel: children clip to it, the overlay around it stays clear
	juce::Component			body { "body" };

	GUI_MipMap				icon;
	GUI_Label				title { ProjectInfo::projectName + juce::String ( " " ) + ProjectInfo::versionString, 20.0f, 700 };
	GUI_Label				copyright { u8"Copyright © 2026 Michael Hartmann (Toddler Boy)", 13.0f, 500 };
	juce::HyperlinkButton	link;

	GUI_ScrollTextViewer	scrollTextViewer;
	GUI_SVG_Button			closeAbout { "close", { "about/close" } };

	melatonin::DropShadow	shadow { 12.0 };

	gin::LayoutSupport	layout { *this };

	juce::Image				background;
	juce::Path				shadowPath;
	juce::Rectangle<int>	bodyBounds;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_About )
};
//-----------------------------------------------------------------------------
