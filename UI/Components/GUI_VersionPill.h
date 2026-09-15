#pragma once

#include <JuceHeader.h>

#include <optional>

#include "ultra-shared/App/AppUpdater.h"
#include "Config/Preferences.h"
#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/Resources/Strings.h"

//-----------------------------------------------------------------------------

// Shows the running version colored by update state. A click starts the
// spinner, the next setState ends it; updating spins with the download
// percentage. Sizes its own width to the content, the layout sets the height

class GUI_VersionPill final : public juce::Button, private juce::AsyncUpdater
{
public:
	GUI_VersionPill ();

	void setState ( AppUpdater::State newState );
	void setProgress ( float newProgress );

	// Spinner on, setState () ends it
	void startChecking ();

	// juce::Button
	void paintButton ( juce::Graphics& g, bool isMouseOver, bool isButtonDown ) override;
	void clicked () override;

	// juce::Component
	void resized () override;
	void lookAndFeelChanged () override;
	std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler () override;

	// juce::SettableTooltipClient (via juce::Button)
	juce::String getTooltip () override;

private:
	// juce::AsyncUpdater: paint detects the spinner's timing, this applies it
	void handleAsyncUpdate () override;

	void showResult ( AppUpdater::State result );
	void applyState ( AppUpdater::State newState );
	void fitToContent ();

	[[ nodiscard ]] bool spinning () const;

	// spoken = for screen readers: versions read digit groups, the product name reads as words
	[[ nodiscard ]] juce::String currentText ( bool spoken = false ) const;
	[[ nodiscard ]] juce::String tooltipText ( bool spoken ) const;
	[[ nodiscard ]] juce::String iconName () const;
	[[ nodiscard ]] float pillWidth () const;

	AppUpdater::State	state = AppUpdater::State::unknown;
	float				progress = 0.0f;

	bool								checking = false;
	std::optional<AppUpdater::State>	pending;
	double								spinStartMS = 0.0;
	double								pulseStartMS = 0.0;

	juce::SharedResourcePointer<Icons>			icons;
	juce::SharedResourcePointer<Preferences>	preferences;
	juce::SharedResourcePointer<Settings>		settings;
	juce::SharedResourcePointer<Strings>		strings;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_VersionPill )
};
//-----------------------------------------------------------------------------
