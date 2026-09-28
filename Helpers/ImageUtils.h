#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

namespace imageutils
{
	struct imageHint
	{
		juce::String	name;
		juce::String	extension;
		int8_t			borderColor = -1;
		bool			firstLuma = false;
		bool			forceNTSC = false;		// PAL unless the picture says so
		bool			loadingScreen = false;
		bool			interlaced = false;		// two fields stacked top to bottom
	};

	[[ nodiscard ]] imageHint hintFromFilename ( const juce::String& in );
	[[ nodiscard ]] juce::String filenameFromHint ( const imageHint& hint );

	[[ nodiscard ]] bool imagesAreEqual ( const juce::Image& a, const juce::Image& b );
	[[ nodiscard ]] bool imagesAreNotEqual ( const juce::Image& a, const juce::Image& b );
}
//-----------------------------------------------------------------------------
