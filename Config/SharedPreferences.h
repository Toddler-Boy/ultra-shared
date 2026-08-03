#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Config/YamlFile.h"

//-----------------------------------------------------------------------------

// The preference defaults both apps share: the whole CRT-emulation block
// (overlay, tv, crt, webcam). Each app's Preferences appends this to its own
// list, so the keys and values cannot drift between the apps

namespace sharedpreferences
{
	[[ nodiscard ]] inline std::vector<YamlFile::value> getDefaultValues ()
	{
		return {
			{ "overlay",	"enabled",				true },
			{ "overlay",	"bitmap",				"C1702 Bedroom" },
			{ "overlay",	"daytime",				35 },
			{ "overlay",	"bezel",				66 },
			{ "overlay",	"shadow",				75 },
			{ "overlay",	"zoom",					50 },
			{ "overlay",	"dust",					50 },
			{ "overlay",	"bloom",				50 },
			{ "overlay",	"chromatic-aberration",	50 },
			{ "overlay",	"grain",				50 },

			{ "tv",			"system",				"AUTO" },
			{ "tv",			"first-luma",			"AUTO" },
			{ "tv",			"brightness",			55 },
			{ "tv",			"contrast",				85 },
			{ "tv",			"saturation",			55 },
			{ "tv",			"overscan",				25 },

			{ "crt",		"emulation",			true },
			{ "crt",		"preset",				"$DATA$/Default" },

			{ "crt",		"jailbars",				50 },

			{ "crt",		"noise",				10 },
			{ "crt",		"sharpening",			30 },
			{ "crt",		"luma-blur",			50 },
			{ "crt",		"chroma-blur",			50 },
			{ "crt",		"crosstalk",			20 },
			{ "crt",		"phase",				22.5f },
			{ "crt",		"hannover",				90 },
			{ "crt",		"rainbowing",			50 },
			{ "crt",		"drift",				20 },

			{ "crt",		"curve",				25 },
			{ "crt",		"rotation",				0 },
			{ "crt",		"bleed",				20 },
			{ "crt",		"bleed-red",			YamlFile::vec2i { -100, 0 } },
			{ "crt",		"bleed-green",			YamlFile::vec2i { 75, -75 } },
			{ "crt",		"bleed-blue",			YamlFile::vec2i { 75, 75 } },
			{ "crt",		"convergence",			20 },
			{ "crt",		"h-wave",				50 },
			{ "crt",		"expansion",			50 },
			{ "crt",		"scanlines",			35 },
			{ "crt",		"scanline-shape",		0 },
			{ "crt",		"mask",					25 },
			{ "crt",		"mask-bitmap",			"Slot Mask" },
			{ "crt",		"phosphor-decay",		60 },

			{ "crt",		"adjacent",				50 },
			{ "crt",		"halation",				50 },
			{ "crt",		"ambient",				50 },

			{ "crt",		"vignette",				50 },
			{ "crt",		"reflection",			50 },

			{ "webcam",		"enabled",				true },
			{ "webcam",		"device",				"" },	// Capture-device name, empty = first camera
			{ "webcam",		"brightness",			50 },
			{ "webcam",		"contrast",				50 },
			{ "webcam",		"saturation",			50 },
			{ "webcam",		"zoom",					0 },
		};
	}
}
//-----------------------------------------------------------------------------
