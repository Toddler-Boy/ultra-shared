#pragma once

#include <JuceHeader.h>

#include <optional>

#include "ultra-shared/App/AppUpdater.h"
#include "Config/Preferences.h"
#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

// Shows the running version colored by update state. A click starts the
// spinner, the next setState ends it; updating spins with the download percentage

class GUI_VersionPill final : public juce::Button
{
public:
	GUI_VersionPill ();

	void setState ( AppUpdater::State newState );
	void setProgress ( float newProgress );

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isMouseOver, bool isButtonDown ) override;
	void clicked () override;

	// juce::Component; the drawn pill is narrower than the bounds
	bool hitTest ( int x, int y ) override;

	// juce::SettableTooltipClient (via juce::Button)
	juce::String getTooltip () override;

private:
	void showResult ( AppUpdater::State result );

	[[ nodiscard ]] bool spinning () const;
	[[ nodiscard ]] juce::String currentText () const;
	[[ nodiscard ]] juce::String iconName () const;
	[[ nodiscard ]] float pillWidth () const;

	AppUpdater::State	state = AppUpdater::State::unknown;
	float				progress = 0.0f;

	bool								checking = false;
	std::optional<AppUpdater::State>	pending;
	double								spinStartMS = 0.0;

	juce::SharedResourcePointer<Icons>			icons;
	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Settings>		settings;
	juce::SharedResourcePointer<Strings>		strings;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_VersionPill )
};
//-----------------------------------------------------------------------------
