#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Config/YamlFile.h"

//-----------------------------------------------------------------------------

class Strings final : public YamlFile
{
public:
	Strings ();

	void setLanguage ( const juce::String& language );

	void load () override;
	[[ nodiscard ]] const juce::String& get ( const juce::String& name );

	// For keys whose absence is normal (per-option display texts): a missing
	// key returns the fallback, silently
	[[ nodiscard ]] juce::String getOptional ( const juce::String& name, const juce::String& fallback ) const;

private:
	juce::String	language = "en";

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( Strings )
};
//-----------------------------------------------------------------------------
