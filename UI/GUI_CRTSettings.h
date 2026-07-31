#pragma once

#include <JuceHeader.h>

#include "Config/Preferences.h"
#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/Video/VIC2_Render.h"

//-----------------------------------------------------------------------------

// The CRT-settings side panel both apps share: builds its widget tree from
// UI/layouts/crt-settings.json and edits the host's overlay/tv/crt/webcam
// preferences; the owning page reacts through the hooks below

class GUI_CRTSettings final
	: public juce::Component
	, private juce::MultiTimer
{
public:
	GUI_CRTSettings ();

	// juce::Component
	void resized () override;

	// Page hooks: push fresh settings into the CRT emulation, react to an
	// overlay/zoom change
	std::function<void ()>	onSettingsChanged;
	std::function<void ()>	onOverlayChanged;
	std::function<void ()>	onZoomChanged;

	// Page context resolving the AUTO choices: the tune's TV standard
	// ("PAL"/"NTSC") and the artwork's first-luma hint
	std::function<juce::String ()>	autoSystem;
	std::function<bool ()>			autoFirstLuma;

	// this
	[[ nodiscard ]] VIC2_Render::settings getVIC2SettingsFromPreferences () const;
	[[ nodiscard ]] lime::CRTEmulation::settings getCRTEmulationSettingsFromPreferences () const;

	// Re-syncs the choice highlights and the palette widget with the preferences
	void updateCRTsettingsUI ();

	void refreshCRTPickLists ();
	void refreshWebcamDevices ();

	// The plain (marker-stripped) name of the selected overlay bitmap
	[[ nodiscard ]] juce::String currentOverlayName () const;

	// The widgets by layout path, for page-side pokes (mouse-wheel zoom)
	[[ nodiscard ]] const std::unordered_map<juce::String, juce::Component*>& componentMap () const	{	return settingsComponentMap;	}

private:
	// juce::MultiTimer
	void timerCallback ( int timerID ) override;

	// this
	void connectComponents ();
	void updateDisablers ();

	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Icons>			icons;

	juce::Viewport	settingsViewport { "viewport" };
	juce::Component	settingsContent { "content" };

	std::unordered_map<juce::String, juce::Component*>	settingsComponentMap;

	// Item id - 1 indexes the marked name ("$DATA$/..." / "$USER$/...") of the
	// overlay and mask pick lists, the form the preferences store
	juce::StringArray	overlayMarkedNames;
	juce::StringArray	maskMarkedNames;

	gin::LayoutSupport	settingsLayout;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_CRTSettings )
};
//-----------------------------------------------------------------------------
