#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Config/YamlFile.h"
//-----------------------------------------------------------------------------

class Icons final : public YamlFile
{
public:
	Icons ();

	void load () override;
	[[ nodiscard ]] const juce::String& get ( const juce::String& name );

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( Icons )
};
//-----------------------------------------------------------------------------
