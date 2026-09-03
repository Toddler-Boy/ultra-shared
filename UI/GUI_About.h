#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Helpers/MipMap.h"

#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_ScrollTextViewer.h"
#include "ultra-shared/UI/GUI_ModalPanel.h"

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

// The About screen both apps share (UI/layouts/about.json sizes the panel).
// Shown/hidden by the host via "showAbout"/"closeAbout"

class GUI_About final : public GUI_ModalPanel
{
public:
	GUI_About ();

	void updateColors ();
	void loadContent ();

private:
	GUI_MipMap				icon;
	GUI_Label				title { ProjectInfo::projectName + juce::String ( " " ) + ProjectInfo::versionString, 20.0f, 700 };
	GUI_Label				copyright { u8"Copyright © 2026 Michael Hartmann (Toddler Boy)", 13.0f, 500 };
	juce::HyperlinkButton	link;

	GUI_ScrollTextViewer	scrollTextViewer;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_About )
};
//-----------------------------------------------------------------------------
