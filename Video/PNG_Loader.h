#pragma once

#include <cstdint>
#include <vector>

//-----------------------------------------------------------------------------

// PNG decoding on top of the libpng copy compiled into juce_graphics, without
// creating a juce::Image (no GPU upload). Paletted files keep their indices
// instead of being expanded to RGB, so the caller can map palette entries once
// rather than matching every pixel

namespace pngloader
{
	struct image
	{
		int		width = 0;
		int		height = 0;

		// Paletted file: one palette index per pixel, palette entries 0xRRGGBB
		bool					paletted = false;
		std::vector<uint8_t>	indices;
		std::vector<uint32_t>	palette;

		// Everything else: one 0xAARRGGBB pixel each
		std::vector<uint32_t>	pixels;

		[[ nodiscard ]] bool isValid () const	{	return width > 0;	}
	};

	[[ nodiscard ]] image decode ( const void* data, const size_t size );
}
//-----------------------------------------------------------------------------
