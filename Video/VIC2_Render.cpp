#include <JuceHeader.h>

#include <cstring>
#include <numbers>

#include "ultra-shared/Config/DataSource.h"

#include "VIC2_Render.h"

//-----------------------------------------------------------------------------

VIC2_Render::VIC2_Render ( const bool withBackup )
{
	indexBuffer = juce::Image ( juce::Image::SingleChannel, outerUnscaledWidth, outerUnscaledHeight, false, juce::SoftwareImageType () );
	yuvBuffer = juce::Image ( juce::Image::ARGB, outerUnscaledWidth, outerUnscaledHeight, false, juce::SoftwareImageType () );
	rgbBuffer = juce::Image ( juce::Image::ARGB, outerUnscaledWidth, outerUnscaledHeight, false, juce::SoftwareImageType () );

	indexPixels = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

	keepBackup = withBackup;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::loadPETSCII ( const char* filename )
{
	indexBufferWidth = 0;
	numFields = 1;
	curField = 0;
	clearCanvas ();

	auto	name = juce::String ( filename );
	auto	file = juce::File ( filename );

	if ( ! file.hasFileExtension ( "petscii" ) )
		return false;

	// Load PETSCII image
	{
		juce::MemoryBlock	mb;
		if ( ! file.loadFileAsData ( mb ) )
			return false;

		// Image has to be 40x25 characters, plus color buffer, border and screen colors (two nibbles in the same byte),
		// and a control-byte (upper- or lower-case)
		if ( mb.getSize () != 40 * 25 * 2 + 1 + 1 )
			return false;

		mb.copyTo ( screenBuffer, 0, 1000 );
		mb.copyTo ( colorBuffer, 1000, 1000 );

		borderCol = uint8_t ( mb[ 2000 ] ) >> 4;	// Unsigned: border colors 8-15 must not sign-extend
		screenCol = mb[ 2000 ] & 0xF;
		controlByte = mb[ 2001 ];
	}

	indexBufferWidth = outerUnscaledWidth;

	renderScreen ();
	backupIndexBuffer ();

	return true;
}
//-----------------------------------------------------------------------------

VIC2_Render::renderStats VIC2_Render::renderScreen ()
{
	// Anything but a cell change forces the full pass
	const auto	full = ! renderCacheValid
					|| screenCol != prevScreenCol
					|| controlByte != prevControlByte
					|| customCharset != prevCharset;

	// Background only on a full pass (which covers the border too), the
	// border alone when its color changed; overlays erase themselves
	if ( full )
		fillAll ( screenCol );

	auto	borderPainted = false;
	if ( full ? borderCol != screenCol : borderCol != prevBorderCol )
	{
		fillBorder ();
		borderPainted = true;
	}

	const auto	lowerCase = ( controlByte >> 1 ) & 0x1;

	// Chargen offset; a custom charset carries its full 2KB set
	auto	characterRom = customCharset ? customCharset : characterData->chargen + lowerCase * 0x800;

	auto	offset = 0;
	auto	dst = reinterpret_cast<uint64_t*> ( indexPixels + ( outerUnscaledWidth * unscaledBorderSizeY + unscaledBorderSizeX ) );

	constexpr auto	dstRowLength = outerUnscaledWidth / 8;
	constexpr auto	rowSkip = unscaledBorderSizeX / 4 + dstRowLength * 7;
	const auto&		bitsToBytes = characterData->bitsToBytes;
	const auto&		colorMasks = characterData->colorMasks;

	const auto	bckCol = colorMasks[ screenCol & 0xF ];
	auto		cellsDrawn = false;

	for ( auto y = 0; y < 25; ++y )
	{
		// Unchanged rows skip wholesale: the common nothing-changed frame is
		// two compares per row instead of forty branchy cells
		if ( ! full
			 && std::memcmp ( screenBuffer + offset, prevScreenBuffer + offset, textColumns ) == 0
			 && std::memcmp ( colorBuffer + offset, prevColorBuffer + offset, textColumns ) == 0 )
		{
			offset += textColumns;
			dst += textColumns + rowSkip;
			continue;
		}

		for ( auto x = 0; x < 40; ++x )
		{
			const auto	character = screenBuffer[ offset ];

			// A full pass skips spaces (the background fill covers them), the
			// dirty pass skips unchanged cells and draws everything else -
			// including spaces, whose blank glyph restores the background
			const auto	skip = full
							 ? character == 32
							 : character == prevScreenBuffer[ offset ]
							   && ( character == 32 || colorBuffer[ offset ] == prevColorBuffer[ offset ] );

			if ( ! skip )
			{
				cellsDrawn = true;

				const auto	colorXor = colorMasks[ colorBuffer[ offset ] & 0xF ] ^ bckCol;
				const auto	src = characterRom + ( character << 3 );

				auto	row = bitsToBytes[ src[ 0 ] ];			dst[ 0				  ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 1 ] ];			dst[ dstRowLength	  ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 2 ] ];			dst[ dstRowLength * 2 ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 3 ] ];			dst[ dstRowLength * 3 ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 4 ] ];			dst[ dstRowLength * 4 ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 5 ] ];			dst[ dstRowLength * 5 ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 6 ] ];			dst[ dstRowLength * 6 ] = bckCol ^ ( row & colorXor );
						row = bitsToBytes[ src[ 7 ] ];			dst[ dstRowLength * 7 ] = bckCol ^ ( row & colorXor );
			}

			++dst;
			++offset;
		}
		dst += rowSkip;
	}

	std::copy_n ( screenBuffer, textColumns * textRows, prevScreenBuffer );
	std::copy_n ( colorBuffer, textColumns * textRows, prevColorBuffer );

	prevScreenCol = screenCol;
	prevBorderCol = borderCol;
	prevControlByte = controlByte;
	prevCharset = customCharset;
	renderCacheValid = true;

	// A rendered text screen fills the full picture, border included
	indexBufferWidth = outerUnscaledWidth;

	return { full, full || borderPainted || cellsDrawn };
}
//-----------------------------------------------------------------------------

void VIC2_Render::placeText ( int x, int y, uint8_t textColor, const char* text )
{
	// Render C64 text with textColor

	// Table for characters higher than 128
	const static char*	asciiConversion =
		"                "		// 0x80 - 0x8F
		"               Y"		// 0x90 - 0x9F
		" !  Y    <      "		// 0xA0 - 0xAF
		"           >   ?"		// 0xB0 - 0xBF
		"AAAAAAACEEEEIIII"		// 0xC0 - 0xCF
		"DNOOOOOXOUUUUY S"		// 0xD0 - 0xDF
		"AAAAAAACEEEEIIII"		// 0xE0 - 0xEF
		"DNOOOOOXOUUUUY Y";		// 0xF0 - 0xFF

	auto	txt = text;

	// More text than the screen holds wraps back to the top rather than running
	// off the end of the buffers
	auto	nextLine = [ &x, &y ]
	{
		x = 0;
		if ( ++y == textRows )
			y = 0;
	};

	while ( auto z = uint8_t ( *txt++ ) )
	{
		// Next line
		if ( z == '\n' )
		{
			nextLine ();
			continue;
		}

		// New text color
		if ( z < 16 )
		{
			textColor = z;
			continue;
		}

		const auto	offset = y * textColumns + x;

		// Cursor
		if ( z == '`' )
		{
			screenBuffer[ offset ] = 160;
			colorBuffer[ offset ] = textColor;
		}
		else if ( z != ' ' )
		{
			if ( z >= 0x80 )
				z = asciiConversion[ z - 0x80 ] + 64;
			else if ( ( controlByte >> 1 ) & 0x1 )
			{
				// Shifted charset: lowercase at 1-26, uppercase stays at 65-90
				if ( z >= 'a' && z <= 'z' )
					z -= 96;
				else if ( z == '@' || ( z >= '[' && z <= '_' ) )
					z -= '@';
			}
			else
			{
				// toUpper for ASCII
				if ( z >= 'a' && z <= 'z' )
					z -= ' ';

				if ( z >= '@' )
					z -= '@';
			}

			screenBuffer[ offset ] = z;
			colorBuffer[ offset ] = textColor;
		}

		if ( ++x == textColumns )
			nextLine ();
	}
}
//-----------------------------------------------------------------------------

void VIC2_Render::setSettings ( const settings& _set )
{
	set = _set;
}
//-----------------------------------------------------------------------------

void VIC2_Render::renderCRT ()
{
	if ( indexBufferWidth == 0 )
		return;

	//
	// Render chroma processed image so thumbnail looks correct
	//
	generateLineYUV ();
	convertToYUVBuffers ();
	chromaProcessing ();
	convertToRGB ();
}
//-----------------------------------------------------------------------------

void VIC2_Render::generateLineYUV ()
{
	const auto	yuv = colo.generateYUV ( set.standard, set.brightness, set.contrast, set.saturation, set.firstLuma, set.warmth );

	// Create Hanover bar palettes
	if ( set.standard == settings::PAL )
	{
		constexpr auto	odd = 360.0f / 16.0f;		// Hanover bar phase-angle

		const auto	oddCos = std::cos ( odd * std::numbers::pi_v<float> / 180.0f );
		const auto	oddSin = std::sin ( odd * std::numbers::pi_v<float> / 180.0f );

		for ( auto i = 0; const auto [ y, u, v ] : yuv )
		{
			lineYUV[ 0 ][ i ] = { y, u * oddCos - v * oddSin * -1.0f,	v * oddCos + u * oddSin * -1.0f };
			lineYUV[ 1 ][ i ] = { y, u * oddCos - v * oddSin,			v * oddCos + u * oddSin };
			++i;
		}
	}
	else
	{
		lineYUV[ 0 ] = yuv;
		lineYUV[ 1 ] = yuv;
	}
}
//-----------------------------------------------------------------------------

void VIC2_Render::convertToYUVBuffers ()
{
	//
	// Convert YUV lines to uint32_t
	//
	uint32_t	yuvColor[ 2 ][ 16 ];

	for ( auto idx = 0; idx < 2; ++idx )
	{
		for ( auto i = 0; i < 16; ++i )
		{
			const auto [ y, u, v ] = lineYUV[ idx ][ i ];

			yuvColor[ idx ][ i ] =		( std::clamp ( int ( y       ), 0, 255 ) << 16 )
									|	( std::clamp ( int ( u + 128 ), 0, 255 ) << 8 )
									|	  std::clamp ( int ( v + 128 ), 0, 255 );
		}
	}

	// Convert image to luma and chroma frame buffer
	const auto	src = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::readOnly ).data;
	const auto	lumaPtr = (uint32_t*)juce::Image::BitmapData ( yuvBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

	auto	pixel = 0;
	for ( auto yIndex = 0; yIndex < outerUnscaledHeight; yIndex++ )
	{
		const auto&	yuv = yuvColor[ yIndex & 1 ];
		for ( auto xIndex = 0; xIndex < outerUnscaledWidth; xIndex++ )
		{
			lumaPtr[ pixel ] = yuv[ src[ pixel ] ];
			++pixel;
		}
	}
}
//-----------------------------------------------------------------------------

void VIC2_Render::chromaProcessing ()
{
	if ( set.standard != settings::colorStandard::PAL )
		return;

	// Apply a one-line delay by blending the chroma buffers onto themselves, 1-pixel offset down
	// Only during PAL emulation, as the "Hanover bars" led to quite visible color-shift every second scanline
	auto	src = (uint32_t*)juce::Image::BitmapData ( yuvBuffer, juce::Image::BitmapData::ReadWriteMode::readWrite ).data;

	const auto	length = outerUnscaledLength;
	const auto	width = outerUnscaledWidth;

	auto	dst = src + length - 1;

	src = dst - width;

	auto	len = length - width;
	while ( len-- )
	{
		const auto	pix1 = *src--;
		const auto	pix2 = *dst;

		const auto	luma = pix2 & 0xFF0000;
		const auto	chromaU = ( ( ( pix1 & 0xFF00 ) + ( pix2 & 0xFF00 ) ) >> 1 ) & 0xFF00;
		const auto	chromaV = ( ( pix1 & 0xFF ) + ( pix2 & 0xFF ) ) >> 1;

		*dst-- = luma | chromaU | chromaV;
	}
}
//-----------------------------------------------------------------------------

void VIC2_Render::convertToRGB ()
{
	const auto	lumaChroma = (uint32_t*)juce::Image::BitmapData ( yuvBuffer, juce::Image::BitmapData::ReadWriteMode::readOnly ).data;
	const auto	imgDataPtr = (uint32_t*)juce::Image::BitmapData ( rgbBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;
	const auto	pxlCount = outerUnscaledLength;

	switch ( set.standard )
	{
		case settings::colorStandard::PAL:
			for ( auto i = 0; i < pxlCount; ++i )
			{
				const auto	pix = lumaChroma[ i ];
				imgDataPtr[ i ] = colo.yuv2rgb ( uint8_t ( pix >> 16 ), ( ( pix >> 8 ) & 0xFF ) - 128.0f, ( pix & 0xFF ) - 128.0f );
			}
			break;

		case settings::colorStandard::NTSC:
			for ( auto i = 0; i < pxlCount; ++i )
			{
				const auto	pix = lumaChroma[ i ];
				imgDataPtr[ i ] = colo.yiq2rgb_sony ( uint8_t ( pix >> 16 ), ( pix & 0xFF ) - 128.0f, ( ( pix >> 8 ) & 0xFF ) - 128.0f );
			}
			break;
	}

	// Add a minimum gray, so it looks more like a real CRT image would
	{
		constexpr auto	ambient = 0.75f;

		constexpr auto	crtAmbient = ambient * ambient;
		constexpr auto	minGray = 0.16f * crtAmbient;

		constexpr auto	crtRed = std::lerp ( 1.0f, 1.0f, crtAmbient ) * minGray;
		constexpr auto	crtGreen = std::lerp ( 0.95f, 1.05f, crtAmbient ) * minGray;
		constexpr auto	crtBlue = std::lerp ( 1.05f, 1.15f, crtAmbient ) * minGray;

		const auto	color = juce::Colour::fromFloatRGBA ( crtRed, crtGreen, crtBlue, 1.0f );

		gin::applyBlend ( rgbBuffer, gin::BlendMode::Lighten, color );
	}
}
//-----------------------------------------------------------------------------

juce::Image VIC2_Render::getThumbnail ()
{
	// Render as CRT image
	renderCRT ();

	if ( numFields == 1 )
		return rgbBuffer;

	// An interlaced picture is seen as the mix of its two fields
	const auto	firstField = rgbBuffer.createCopy ();

	nextField ();
	restoreIndexBuffer ();
	renderCRT ();

	nextField ();
	restoreIndexBuffer ();

	const auto	src = (const uint32_t*)juce::Image::BitmapData ( firstField, juce::Image::BitmapData::ReadWriteMode::readOnly ).data;
	const auto	dst = (uint32_t*)juce::Image::BitmapData ( rgbBuffer, juce::Image::BitmapData::ReadWriteMode::readWrite ).data;

	for ( auto i = 0; i < outerUnscaledLength; ++i )
	{
		const auto	a = src[ i ];
		const auto	b = dst[ i ];

		dst[ i ] = ( ( ( a ^ b ) & 0xFEFEFEFEu ) >> 1 ) + ( a & b );
	}

	return rgbBuffer;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::wasBorderFilled () const
{
	return		indexBufferWidth == outerUnscaledWidth
			||	borderCol != vic2::black
			||	borderInFilename;
}
//-----------------------------------------------------------------------------

VIC2_Render_Data::VIC2_Render_Data ()
{
	// Load font-data
	{
		const auto	mb = datasource::loadData ( "Roms/chargen.bin" );
		if ( mb.getSize () != 0 )
			std::copy_n ( (const uint8_t*)mb.getData (), std::min ( mb.getSize (), sizeof ( chargen ) ), chargen );
	}

	// Pre-calculate bit-to-byte conversion for all possible combinations of foreground and background color and 8-bit character data
	{
		for ( auto charData = 0; charData < 256; ++charData )
		{
			auto	result = static_cast<uint64_t> ( ( charData & 0x80 ) ? 0xff : 0x00 );

			result |= static_cast<uint64_t> ( ( charData & 0x40 ) ? 0xff : 0x00 ) << 8;
			result |= static_cast<uint64_t> ( ( charData & 0x20 ) ? 0xff : 0x00 ) << 16;
			result |= static_cast<uint64_t> ( ( charData & 0x10 ) ? 0xff : 0x00 ) << 24;
			result |= static_cast<uint64_t> ( ( charData & 0x08 ) ? 0xff : 0x00 ) << 32;
			result |= static_cast<uint64_t> ( ( charData & 0x04 ) ? 0xff : 0x00 ) << 40;
			result |= static_cast<uint64_t> ( ( charData & 0x02 ) ? 0xff : 0x00 ) << 48;
			result |= static_cast<uint64_t> ( ( charData & 0x01 ) ? 0xff : 0x00 ) << 56;

			bitsToBytes[ charData ] = result;
		}
	}

	// Pre-spread every palette index over 8 bytes
	for ( auto index = 0; index < 16; ++index )
	{
		auto	mask = uint64_t ( index );
		mask |= mask << 8;
		mask |= mask << 16;
		mask |= mask << 32;

		colorMasks[ index ] = mask;
	}
}
//-----------------------------------------------------------------------------
