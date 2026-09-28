#pragma once

#include <JuceHeader.h>

#include "colodore.h"

//-----------------------------------------------------------------------------

namespace colormatch
{
	// Match image colors to vic2-palette indices, one per input color. Greys go by luma
	// alone, colors by hue on the PAL chroma circle with luma only splitting a hue pair
	[[ nodiscard ]] std::vector<uint8_t> matchToVIC2 ( const colodore& colo, const std::vector<uint32_t>& imgPalette, const colodore::rgbPalette& refPalette );
}
//-----------------------------------------------------------------------------
