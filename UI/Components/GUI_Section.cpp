#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_Section.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

void GUI_Section::paint ( juce::Graphics& g )
{
	g.setColour ( UI::getShade ( 0.145f ) );
	g.fillRoundedRectangle ( getLocalBounds ().toFloat (), corner );
}
//-----------------------------------------------------------------------------
