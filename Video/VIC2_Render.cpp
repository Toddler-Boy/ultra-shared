#include <JuceHeader.h>

#include <cstring>
#include <numbers>

#include "ultra-shared/Config/DataSource.h"
#include "ultra-shared/Helpers/ImageUtils.h"

#include "PNG_Loader.h"
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

bool VIC2_Render::loadImage ( const char* filename )
{
	indexBufferWidth = 0;

	juce::MemoryBlock	mb;
	if ( ! juce::File ( filename ).loadFileAsData ( mb ) )
		return false;

	return loadImage ( filename, mb.getData (), mb.getSize () );
}
//-----------------------------------------------------------------------------

bool VIC2_Render::loadImage ( const char* filename, const void* data, const size_t size )
{
	indexBufferWidth = 0;
	numFields = 1;
	curField = 0;
	multicolor = false;

	if ( ! juce::String ( filename ).endsWithIgnoreCase ( ".png" ) )
		return false;

	const auto	image = pngloader::decode ( data, size );

	if ( image.paletted )
		return convertPaletted ( filename, image );

	if ( image.isValid () )
		return convertTrueColor ( filename, image.pixels.data (), image.width, image.height );

	return false;
}
//-----------------------------------------------------------------------------

void VIC2_Render::fillBorder ()
{
	auto	dst = indexPixels;

	// Fill top-border
	std::fill_n ( dst, outerUnscaledWidth * unscaledBorderSizeY, borderCol );
	dst += outerUnscaledWidth * unscaledBorderSizeY;

	for ( auto y = 0; y < innerUnscaledHeight; ++y )
	{
		// Fill left-border
		std::fill_n ( dst, unscaledBorderSizeX, borderCol );
		dst += unscaledBorderSizeX + innerUnscaledWidth;

		// Fill right-border
		std::fill_n ( dst, unscaledBorderSizeX, borderCol );
		dst += unscaledBorderSizeX;
	}

	// Fill bottom-border
	std::fill_n ( dst, outerUnscaledWidth * unscaledBorderSizeY, borderCol );
}
//-----------------------------------------------------------------------------

void VIC2_Render::fillAll ( const uint8_t innerCol )
{
	// Fill background with one color
	std::fill_n ( indexPixels, outerUnscaledLength, innerCol );
}
//-----------------------------------------------------------------------------

// Match image colors to vic2-palette indices, one per input color. Distinct source
// colors get distinct indices while each stays within maxDetour of its nearest; beyond that they share
static std::vector<uint8_t> matchToVIC2 ( const std::vector<uint32_t>& imgPalette, const colodore::rgbPalette& refPalette )
{
	// Screenshot survey: genuine detours stay below 850, forced ones start above 1050
	constexpr auto	maxDetour = 1000;

	// Dwarfs any sum of in-budget distances, so fewer shares always win
	constexpr auto	sharePenalty = 1 << 24;

	// Distance function
	auto distanceRGB = [] ( const uint32_t imgCol, const uint32_t refCol )
	{
		const auto	imgR = ( imgCol >> 16 ) & 0xFF;
		const auto	imgG = ( imgCol >> 8 ) & 0xFF;
		const auto	imgB = imgCol & 0xFF;

		const auto	refR = ( refCol >> 16 ) & 0xFF;
		const auto	refG = ( refCol >> 8 ) & 0xFF;
		const auto	refB = refCol & 0xFF;

		auto pow2 = [] ( const int input1, const int input2, const float weight )
		{
			const auto	input = std::abs ( input2 - input1 );
			return int ( ( input * input ) * weight );
		};

		return pow2 ( imgR, refR, 0.299f ) + pow2 ( imgG, refG, 0.587f ) + pow2 ( imgB, refB, 0.114f );
	};

	const auto	numColors = int ( imgPalette.size () );
	const auto	numIndices = int ( refPalette.size () );

	std::array<std::array<int, 16>, 16>	cost {};
	for ( auto pos = 0; pos < numColors; ++pos )
	{
		auto&	row = cost[ pos ];
		for ( auto index = 0; index < numIndices; ++index )
			row[ index ] = distanceRGB ( imgPalette[ pos ], refPalette[ index ] );

		const auto	limit = *std::ranges::min_element ( row ) + maxDetour;
		for ( auto& c : row )
			if ( c > limit )
				c = INT_MAX;
	}

	// dp[mask] = cheapest assignment of the colors so far using exactly the indices in mask;
	// step records per color and mask the index taken, high bit set when it was shared
	const auto	numMasks = size_t ( 1 ) << numIndices;

	std::vector<int>		dp ( numMasks, INT_MAX );
	std::vector<int>		next ( numMasks );
	std::vector<uint8_t>	step ( numMasks * size_t ( numColors ) );

	constexpr uint8_t	sharedBit = 0x80;

	dp[ 0 ] = 0;
	for ( auto pos = 0; pos < numColors; ++pos )
	{
		std::ranges::fill ( next, INT_MAX );

		for ( size_t mask = 0; mask < numMasks; ++mask )
		{
			if ( dp[ mask ] == INT_MAX )
				continue;

			for ( auto index = 0; index < numIndices; ++index )
			{
				if ( cost[ pos ][ index ] == INT_MAX )
					continue;

				const auto	shared = ( mask & ( 1u << index ) ) != 0;
				const auto	total = dp[ mask ] + cost[ pos ][ index ] + ( shared ? sharePenalty : 0 );
				const auto	nextMask = mask | ( 1u << index );

				if ( total < next[ nextMask ] )
				{
					next[ nextMask ] = total;
					step[ size_t ( pos ) * numMasks + nextMask ] = uint8_t ( index ) | ( shared ? sharedBit : 0 );
				}
			}
		}

		std::swap ( dp, next );
	}

	// Cheapest complete assignment, then walk it backwards to recover each color's index
	auto	mask = size_t ( std::ranges::min_element ( dp ) - dp.begin () );

	std::vector<uint8_t>	out ( imgPalette.size () );
	for ( auto pos = numColors - 1; pos >= 0; --pos )
	{
		const auto	taken = step[ size_t ( pos ) * numMasks + mask ];
		const auto	index = uint8_t ( taken & ~sharedBit );

		out[ size_t ( pos ) ] = index;

		if ( ! ( taken & sharedBit ) )
			mask &= ~( size_t ( 1 ) << index );
	}
	return out;
}
//-----------------------------------------------------------------------------

// How many C64 pictures a PNG of this size stacks: one at 320x200 or
// 384x272, two interlace fields at doubled height, 0 for anything else
static int fieldsForSize ( const int width, const int height )
{
	const auto	frameHeight =	width == VIC2_Render::innerUnscaledWidth ? VIC2_Render::innerUnscaledHeight
							:	width == VIC2_Render::outerUnscaledWidth ? VIC2_Render::outerUnscaledHeight : 0;

	if ( frameHeight == 0 || height % frameHeight != 0 || height > frameHeight * 2 )
		return 0;

	return height / frameHeight;
}
//-----------------------------------------------------------------------------

template <typename T, typename F>
void VIC2_Render::storeFields ( const char* filename, const T* src, const int width, const int fields, F convert )
{
	const auto	inner = width == innerUnscaledWidth;
	const auto	height = inner ? innerUnscaledHeight : outerUnscaledHeight;
	const auto	rowByteSkip = inner ? unscaledBorderSizeX * 2 : 0;

	numFields = fields;
	multicolor = true;

	for ( auto field = fields - 1; field >= 0; --field )
	{
		auto	dst = indexPixels + ( inner ? unscaledBorderSizeY * outerUnscaledWidth + unscaledBorderSizeX : 0 );
		auto	s = src + size_t ( field ) * size_t ( width ) * size_t ( height );

		for ( auto y = 0; y < height; ++y )
		{
			for ( auto x = 0; x < width; ++x )
				*dst++ = convert ( *s++ );

			dst += rowByteSkip;
		}

		if ( hasHiresPixels () )
			multicolor = false;

		// The picture replaced whatever renderScreen drew
		invalidate ();
		indexBufferWidth = width;

		findBorderColor ( filename );

		curField = field;
		backupIndexBuffer ();
	}
}
//-----------------------------------------------------------------------------

bool VIC2_Render::convertTrueColor ( const char* filename, const uint32_t* rawData, const int width, const int height )
{
	indexBufferWidth = 0;

	const auto	fields = fieldsForSize ( width, height );
	if ( fields == 0 )
		return false;

	// Convert true color image (32-bit, alpha gets ignored) to vic2-palette indices (0-15)

	// Create palette to match to. Getting better results with brighter, more saturated colors (PAL)
	auto	yuv = colo.generateYUV ( settings::colorStandard::PAL, 60.0f, 100.0f, 55.0f );
	auto	referencePalette = colo.generateRGB ( settings::colorStandard::PAL, yuv );

	//
	// First pass: build palette of the image
	//
	std::vector<uint32_t>	imagePalette;

	for ( auto i = 0; i < ( width * height ); ++i )
	{
		// Look for color in map
		const auto	col = rawData[ i ];
		if ( std::find ( imagePalette.begin (), imagePalette.end (), col ) != imagePalette.end () )
			continue;

		imagePalette.emplace_back ( col );

		jassert ( imagePalette.size () <= 16 );
		if ( imagePalette.size () > 16 )
			return false;
	}

	//
	// Second pass: map image palette to vic2-palette indices
	//
	const auto	vic2Indices = matchToVIC2 ( imagePalette, referencePalette );

	std::unordered_map<uint32_t, uint8_t>	mapped;
	for ( size_t i = 0; i < imagePalette.size (); ++i )
		mapped[ imagePalette[ i ] ] = vic2Indices[ i ];

	//
	// Third pass: create index buffer
	//
	storeFields ( filename, rawData, width, fields, [ &mapped ] ( const uint32_t col ) { return mapped[ col ]; } );

	return true;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::convertPaletted ( const char* filename, const pngloader::image& img )
{
	indexBufferWidth = 0;

	const auto	fields = fieldsForSize ( img.width, img.height );
	if ( fields == 0 )
		return false;

	// Collect the palette entries the image actually uses; files routinely
	// carry a full 256-entry palette with only a handful referenced
	std::array<bool, 256>	used {};
	for ( const auto index : img.indices )
		used[ index ] = true;

	// An index past the palette means the file is broken
	for ( auto i = int ( img.palette.size () ); i < 256; ++i )
		if ( used[ i ] )
			return false;

	std::vector<uint32_t>		colors;
	std::array<uint8_t, 256>	colorPos {};

	for ( auto i = 0; i < int ( img.palette.size () ); ++i )
	{
		if ( ! used[ i ] )
			continue;

		const auto	col = img.palette[ i ];

		// Entries sharing one RGB value collapse onto one color
		if ( const auto it = std::find ( colors.begin (), colors.end (), col ); it != colors.end () )
		{
			colorPos[ i ] = uint8_t ( it - colors.begin () );
			continue;
		}

		colorPos[ i ] = uint8_t ( colors.size () );
		colors.emplace_back ( col );

		jassert ( colors.size () <= 16 );
		if ( colors.size () > 16 )
			return false;
	}

	// Create palette to match to. Getting better results with brighter, more saturated colors (PAL)
	const auto	yuv = colo.generateYUV ( settings::colorStandard::PAL, 60.0f, 100.0f, 55.0f );
	const auto	referencePalette = colo.generateRGB ( settings::colorStandard::PAL, yuv );

	const auto	vic2Indices = matchToVIC2 ( colors, referencePalette );

	// Palette-index to vic2-index blit table
	std::array<uint8_t, 256>	lut {};
	for ( auto i = 0; i < int ( img.palette.size () ); ++i )
		if ( used[ i ] )
			lut[ i ] = vic2Indices[ colorPos[ i ] ];

	storeFields ( filename, img.indices.data (), img.width, fields, [ &lut ] ( const uint8_t index ) { return lut[ index ]; } );

	return true;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::loadPETSCII ( const char* filename )
{
	indexBufferWidth = 0;
	numFields = 1;
	curField = 0;
	multicolor = false;

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

void VIC2_Render::findBorderColor ( const char* _filename )
{
	// Source image already has a border
	if ( indexBufferWidth == outerUnscaledWidth )
		return;

	// Black is the default
	borderCol = vic2::num_colors;
	borderInFilename = false;

	// Get border color from filename
	{
		const auto	hint = imageutils::hintFromFilename ( _filename );

		if ( hint.borderColor >= 0 )
			borderCol = uint8_t ( hint.borderColor );
	}

	borderInFilename = borderCol < vic2::num_colors;
	if ( ! borderInFilename )
		borderCol = vic2::black;

	fillBorder ();
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

bool VIC2_Render::hasHiresPixels () const
{
	for ( auto y = 0; y < innerUnscaledHeight; ++y )
	{
		const auto*	row = indexPixels + ( y + unscaledBorderSizeY ) * outerUnscaledWidth + unscaledBorderSizeX;

		for ( auto x = 0; x < innerUnscaledWidth; x += 2 )
			if ( row[ x ] != row[ x + 1 ] )
				return true;
	}

	return false;
}
//-----------------------------------------------------------------------------

void VIC2_Render::backupIndexBuffer ()
{
	// Interlace fields are kept even without a backup, the thumbnail
	// needs both
	if ( ! keepBackup && numFields == 1 )
		return;

	const auto	needed = size_t ( numFields ) * outerUnscaledLength;
	if ( indexBufferBackup.size () < needed )
		indexBufferBackup.resize ( needed );

	std::copy_n ( indexPixels, outerUnscaledLength, indexBufferBackup.data () + size_t ( curField ) * outerUnscaledLength );
}
//-----------------------------------------------------------------------------

void VIC2_Render::restoreIndexBuffer ()
{
	if ( indexBufferBackup.empty () )
		return;

	std::copy_n ( indexBufferBackup.data () + size_t ( curField ) * outerUnscaledLength, outerUnscaledLength, indexPixels );
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
