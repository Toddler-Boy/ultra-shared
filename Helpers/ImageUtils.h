#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

namespace imageutils
{
	// What a screenshot shows, one letter in the hint: T, G or L
	enum class screenKind : uint8_t { none, title, game, loading };

	struct imageHint
	{
		juce::String	name;
		juce::String	extension;
		int8_t			borderColor = -1;
		bool			firstLuma = false;
		bool			forceNTSC = false;		// PAL unless the picture says so
		screenKind		kind = screenKind::none;
	};

	[[ nodiscard ]] imageHint hintFromFilename ( const juce::String& in );
	[[ nodiscard ]] juce::String filenameFromHint ( const imageHint& hint );

	[[ nodiscard ]] bool imagesAreEqual ( const juce::Image& a, const juce::Image& b );
	[[ nodiscard ]] bool imagesAreNotEqual ( const juce::Image& a, const juce::Image& b );
}
//-----------------------------------------------------------------------------
