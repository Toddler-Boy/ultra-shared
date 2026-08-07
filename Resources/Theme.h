#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Config/YamlFile.h"
#include "UI/ui-corners.h"
#include "UI/ui-fonts.h"
#include "UI/ui-lines.h"
#include "UI/ui-paddings.h"

//-----------------------------------------------------------------------------

class Theme final : public YamlFile
{
public:
	Theme ();

	void setTargetLAF ( juce::LookAndFeel& _laf )	{	laf = &_laf;	}

	using YamlFile::load;

	// Loads a theme by its marked name; a missing file leaves the code
	// defaults in place. This needs a target juce::LookAndFeel to work
	void load ( const juce::String& name );

	// User-side color grading, applied on top of whatever theme is loaded.
	// All 1.0 = the theme's colors bit-identical
	struct ColorAdjustments
	{
		float	gamma = 1.0f;
		float	brightness = 1.0f;
		float	contrast = 1.0f;
		float	saturation = 1.0f;

		[[ nodiscard ]] bool isNeutral () const
		{
			return	   juce::approximatelyEqual ( gamma, 1.0f )
					&& juce::approximatelyEqual ( brightness, 1.0f )
					&& juce::approximatelyEqual ( contrast, 1.0f )
					&& juce::approximatelyEqual ( saturation, 1.0f );
		}
	};

	// Stores the adjustments and re-applies the loaded colors through them;
	// the caller refreshes whatever it derives from the palette afterwards
	void setColorAdjustments ( const ColorAdjustments& adj );

	// A copy of the color with the adjustments applied
	[[ nodiscard ]] static juce::Colour adjust ( const juce::Colour col, const ColorAdjustments& adj );

	void setUserRoot ( const juce::File& _userRoot );

	// Maps a marked theme name ("$DATA$/default", "$USER$/neon") to its yml
	// file. Empty when that root is not set up yet
	[[ nodiscard ]] juce::File resolve ( const juce::String& markedName ) const;

	// Every theme as a marked name: the factory group first, then the user
	// group, each alphabetical
	[[ nodiscard ]] juce::StringArray listThemes () const;

	[[ nodiscard ]] float getCornerRadius ( const UI::corners::Role role ) const	{	return cornerRadius[ static_cast<size_t> ( role ) ];	}
	[[ nodiscard ]] float getLineWidth ( const UI::lines::Role role ) const		{	return lineWidths[ static_cast<size_t> ( role ) ];		}

	[[ nodiscard ]] const UI::paddings::Def& getPaddingDef ( const UI::paddings::Role role ) const	{	return paddingDefs[ static_cast<size_t> ( role ) ];	}

	[[ nodiscard ]] const UI::fonts::Def& getFontDef ( const UI::fonts::Role role ) const	{	return fontDefs[ static_cast<size_t> ( role ) ];	}

private:
	// Resets every registry to its code defaults: the constructor runs this
	// so components can query fonts and corners during construction, before
	// the first theme load, and load () runs it so a theme switch never
	// inherits values from the previous file
	void resetDefaults ();

	// The loaded, untransformed palette (colourId to color); applyColors runs
	// it through the adjustments into the LookAndFeel
	void applyColors ();

	std::vector<std::pair<int, juce::Colour>>	rawColors;
	ColorAdjustments							adjustments;

	std::array<float, static_cast<size_t> ( UI::corners::count )>				cornerRadius;
	std::array<float, static_cast<size_t> ( UI::lines::count )>					lineWidths;
	std::array<UI::paddings::Def, static_cast<size_t> ( UI::paddings::count )>	paddingDefs;
	std::array<UI::fonts::Def, static_cast<size_t> ( UI::fonts::count )>		fontDefs;

	juce::File			userRoot;
	juce::LookAndFeel*	laf = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( Theme )
};
//-----------------------------------------------------------------------------
