#pragma once

#include <cstdint>

//-----------------------------------------------------------------------------

namespace pictureanalyzer
{
	enum flags : uint16_t
	{
		multicolor		= 1 << 0,
		borderSprites	= 1 << 1,
		rasterSplits	= 1 << 2,		// border colour changes per raster line or cycle
	};

	// VIC palette indices, one byte per pixel
	struct picture
	{
		const uint8_t*	pixels = nullptr;
		int				width = 0;
		int				height = 0;
		int				stride = 0;

		// The whole 384x272 frame when the file carried its border
		const uint8_t*	frame = nullptr;
	};

	[[ nodiscard ]] uint16_t analyze ( const picture& pic );

	// Both interlace fields make one picture
	[[ nodiscard ]] uint16_t combine ( const uint16_t a, const uint16_t b );
}
//-----------------------------------------------------------------------------
