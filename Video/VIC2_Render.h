#pragma once

#include <JuceHeader.h>

#include "colodore.h"

namespace pngloader { struct image; }

//-----------------------------------------------------------------------------

enum vic2 : uint8_t
{
	black,
	white,
	red,
	cyan,
	purple,
	green,
	blue,
	yellow,
	orange,
	brown,
	light_red,
	dark_grey,
	grey,
	light_green,
	light_blue,
	light_grey,

	num_colors
};

namespace VIC2
{
	// PAL pixel-aspect-ratio correction: VIC-II pixels are not square, so a
	// 320-wide bitmap displays as 320 * 0.937 = ~300 "square" pixels wide
	constexpr auto	truePalX = 0.937f;
}
//-----------------------------------------------------------------------------

class VIC2_Render_Data final
{
public:
	VIC2_Render_Data ();

	uint8_t		chargen[ 4096 ];
	uint64_t	bitsToBytes[ 256 ];
	uint64_t	colorMasks[ 16 ];		// the palette index spread over 8 bytes
};
//-----------------------------------------------------------------------------

class VIC2_Render final
{
public:
	VIC2_Render ( const bool withBackup );

	// Load image from file (*.png)
	bool loadImage ( const char* filename );

	// Same, with the bytes supplied by the caller (factory data from the pak);
	// the name is still needed for its extension and the conversion hints
	bool loadImage ( const char* filename, const void* data, const size_t size );

	// Image is pure PETSCII, 40x25 characters, plus color buffer, border and screen colors and a control-byte (upper- or lower-case)
	[[ nodiscard ]] bool loadPETSCII ( const char* filename );

	// Convert screen- and color-buffers, etc. to an index buffer. A full pass
	// repaints over any pixel overlays; changed says whether any pixel moved
	struct renderStats
	{
		bool	full = false;
		bool	changed = false;
	};

	renderStats renderScreen ();

	struct settings
	{
		enum colorStandard : int8_t { PAL, NTSC };
		colorStandard	standard = PAL;

		bool	firstLuma = false;

		// All three values between 0.0 and 100.0
		float	brightness = 50.0f;
		float	contrast = 100.0f;
		float	saturation = 50.0f;

		// -100.0 cold .. +100.0 warm, chroma only, greys stay pure
		float	warmth = 0.0f;

		bool	raw = false;

		[[ nodiscard ]] bool needsNewPalette ( const settings& other ) const
		{
			return firstLuma != other.firstLuma || raw != other.raw || warmth != other.warmth;
		}
	};

	// Image gets generated from text
	void generateTextCRT ( const uint8_t bckColors, uint8_t textColor, const char* text );

	// Write text into the screen/color buffers without rendering: '\n' starts
	// a new line, bytes below 16 switch the text color, '`' is the cursor block.
	// The shifted charset (controlByte) keeps the ASCII case, the unshifted one
	// folds to uppercase
	void placeText ( int x, int y, uint8_t textColor, const char* text );

	// Replace the ROM chargen with a custom 2KB set (256 glyphs, screen-code
	// order); nullptr restores the ROM. The caller keeps the bits alive
	void setCustomCharset ( const uint8_t* bits )	{	customCharset = bits;	}

	// renderScreen only redraws cells whose character or color changed; force
	// a full pass when the index buffer was written outside of it
	void invalidate ()	{	renderCacheValid = false;	}

	// Create images for CRT-emulation and thumbnail
	void setSettings ( const settings& set );
	void renderCRT ();

	[[ nodiscard ]] juce::Image& getCRT () { return indexBuffer; }

	// The index buffer's pixels; the software image's data never moves
	[[ nodiscard ]] uint8_t* getIndexPixels ()	{	return indexPixels;	}
	[[ nodiscard ]] juce::Image getThumbnail ();
	[[ nodiscard ]] bool wasBorderFilled () const;

	void restoreIndexBuffer ();

	// C64 image size (without borders)
	static constexpr auto	innerUnscaledWidth = 320;
	static constexpr auto	innerUnscaledHeight = 200;

	// C64 border size
	static constexpr auto	unscaledBorderSizeX = 32;
	static constexpr auto	unscaledBorderSizeY = 36;

	static constexpr auto	outerUnscaledWidth = innerUnscaledWidth + unscaledBorderSizeX * 2;
	static constexpr auto	outerUnscaledHeight = innerUnscaledHeight + unscaledBorderSizeY * 2;

	static constexpr auto	outerUnscaledLength = outerUnscaledWidth * outerUnscaledHeight;

	static constexpr auto	textColumns = 40;
	static constexpr auto	textRows = 25;

	uint8_t		screenBuffer[ textColumns * textRows ];
	uint8_t		colorBuffer[ textColumns * textRows ];
	uint8_t		screenCol = vic2::black;
	uint8_t		borderCol = vic2::black;
	uint8_t		controlByte = 0x15;

private:
	// Image has to be 32-bit per pixel, alpha is ignored
	[[ nodiscard ]] bool convertTrueColor ( const char* filename, const uint32_t* rawData, const int width, const int height );

	// Paletted image straight from the decoder, pixels stay palette indices
	// and only the palette entries get matched
	[[ nodiscard ]] bool convertPaletted ( const char* filename, const pngloader::image& img );

	juce::SharedResourcePointer<VIC2_Render_Data>	characterData;

	void findBorderColor ( const char* filename );
	void fillBorder ();
	void fillAll ( const uint8_t innerCol );

	colodore				colo;
	colodore::yuvPalette	lineYUV[ 2 ];

	void generateLineYUV ();
	void convertToYUVBuffers ();
	void chromaProcessing ();
	void convertToRGB ();

	settings	set;

	const uint8_t*			customCharset = nullptr;

	// The state of the last renderScreen pass, for the dirty-cell check
	uint8_t			prevScreenBuffer[ textColumns * textRows ];
	uint8_t			prevColorBuffer[ textColumns * textRows ];
	uint8_t			prevScreenCol = 0;
	uint8_t			prevBorderCol = 0;
	uint8_t			prevControlByte = 0;
	const uint8_t*	prevCharset = nullptr;
	bool			renderCacheValid = false;

	int						indexBufferWidth = 0;
	juce::Image				indexBuffer;
	uint8_t*				indexPixels = nullptr;
	juce::Image				yuvBuffer;
	juce::Image				rgbBuffer;

	void backupIndexBuffer ();
	std::vector<uint8_t>	indexBufferBackup;

	bool		borderInFilename = false;
};
//-----------------------------------------------------------------------------
