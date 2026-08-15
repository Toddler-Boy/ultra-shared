#include <JuceHeader.h>

#include <bit>
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

	if ( withBackup )
		indexBufferBackup.resize ( outerUnscaledLength );
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
	auto	dst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

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
	auto	dst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

	std::fill_n ( dst, outerUnscaledLength, innerCol );
}
//-----------------------------------------------------------------------------

// Match image colors to vic2-palette indices. Distinct source colors mean
// distinct hardware indices, so pick the one-to-one assignment with the
// smallest total distance over all colors; returns one index per input color
static std::vector<uint8_t> matchToVIC2 ( const std::vector<uint32_t>& imgPalette, const colodore::rgbPalette& refPalette )
{
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
		for ( auto index = 0; index < numIndices; ++index )
			cost[ pos ][ index ] = distanceRGB ( imgPalette[ pos ], refPalette[ index ] );

	// dp[mask] = cheapest assignment of the first popcount(mask) colors to exactly the indices in mask;
	// masks only grow, so ascending order finalizes each entry before it is extended
	std::vector<int>		dp ( size_t ( 1 ) << numIndices, INT_MAX );
	std::vector<uint8_t>	lastIndex ( size_t ( 1 ) << numIndices, 0 );

	dp[ 0 ] = 0;
	for ( auto mask = 0; mask < int ( dp.size () ); ++mask )
	{
		if ( dp[ mask ] == INT_MAX )
			continue;

		const auto	pos = std::popcount ( unsigned ( mask ) );
		if ( pos >= numColors )
			continue;

		for ( auto index = 0; index < numIndices; ++index )
		{
			if ( mask & ( 1 << index ) )
				continue;

			const auto	next = mask | ( 1 << index );
			if ( const auto total = dp[ mask ] + cost[ pos ][ index ]; total < dp[ next ] )
			{
				dp[ next ] = total;
				lastIndex[ next ] = uint8_t ( index );
			}
		}
	}

	// Cheapest complete assignment, then walk it backwards to recover each color's index
	auto	bestMask = 0;
	auto	bestTotal = INT_MAX;
	for ( auto mask = 0; mask < int ( dp.size () ); ++mask )
		if ( std::popcount ( unsigned ( mask ) ) == numColors && dp[ mask ] < bestTotal )
		{
			bestTotal = dp[ mask ];
			bestMask = mask;
		}

	std::vector<uint8_t>	out ( imgPalette.size () );
	for ( auto mask = bestMask; mask; )
	{
		const auto	index = lastIndex[ mask ];
		out[ size_t ( std::popcount ( unsigned ( mask ) ) - 1 ) ] = index;
		mask &= ~( 1 << index );
	}
	return out;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::convertTrueColor ( const char* filename, const uint32_t* rawData, const int width, const int height )
{
	indexBufferWidth = 0;

	// Images has to either 320x200 or 384x272
	if ( ! ( ( width == innerUnscaledWidth && height == innerUnscaledHeight ) || ( width == outerUnscaledWidth && height == outerUnscaledHeight ) ) )
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
	{
		auto	dst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

		const auto	rowByteSkip = ( width == innerUnscaledWidth ) ? unscaledBorderSizeX * 2 : 0;
		if ( rowByteSkip )
			dst += unscaledBorderSizeY * outerUnscaledWidth + unscaledBorderSizeX;

		for ( auto y = 0; y < height; ++y )
		{
			for ( auto x = 0; x < width; ++x )
				*dst++ = mapped[ *rawData++ ];

			dst += rowByteSkip;
		}
	}

	// End of conversion
	indexBufferWidth = width;

	findBorderColor ( filename );

	backupIndexBuffer ();

	return true;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::convertPaletted ( const char* filename, const pngloader::image& img )
{
	indexBufferWidth = 0;

	// Images has to either 320x200 or 384x272
	if ( ! ( ( img.width == innerUnscaledWidth && img.height == innerUnscaledHeight ) || ( img.width == outerUnscaledWidth && img.height == outerUnscaledHeight ) ) )
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

	// Create index buffer
	{
		auto	dst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;
		auto	src = img.indices.data ();

		const auto	rowByteSkip = ( img.width == innerUnscaledWidth ) ? unscaledBorderSizeX * 2 : 0;
		if ( rowByteSkip )
			dst += unscaledBorderSizeY * outerUnscaledWidth + unscaledBorderSizeX;

		for ( auto y = 0; y < img.height; ++y )
		{
			for ( auto x = 0; x < img.width; ++x )
				*dst++ = lut[ *src++ ];

			dst += rowByteSkip;
		}
	}

	// End of conversion
	indexBufferWidth = img.width;

	findBorderColor ( filename );

	backupIndexBuffer ();

	return true;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::loadPETSCII ( const char* filename )
{
	indexBufferWidth = 0;

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

void VIC2_Render::renderScreen ()
{
	// Border and screen colors
	fillAll ( screenCol );
	if ( screenCol != borderCol )
		fillBorder ();

	const auto	lowerCase = ( controlByte >> 1 ) & 0x1;

	// Chargen offset
	auto	characterRom = characterData->chargen + lowerCase * 0x800;
	auto	srcDst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

	auto	offset = 0;
	auto	dst = reinterpret_cast<uint64_t*> ( srcDst + ( outerUnscaledWidth * unscaledBorderSizeY + unscaledBorderSizeX ) );

	constexpr auto	dstRowLength = outerUnscaledWidth / 8;
	constexpr auto	rowSkip = unscaledBorderSizeX / 4 + dstRowLength * 7;
	const auto&		bitsToBytes = characterData->bitsToBytes;

	auto getColMask = [] ( const uint8_t color )
	{
		auto	col = uint64_t ( color & 0xF );
		col |= col << 8;
		col |= col << 16;
		col |= col << 32;
		return col;
	};

	const auto	bckCol = getColMask ( screenCol );

	for ( auto y = 0; y < 25; ++y )
	{
		for ( auto x = 0; x < 40; ++x )
		{
			const auto	character = screenBuffer[ offset ];

			// Space
			if ( character != 32 )
			{
				const auto	color = getColMask ( colorBuffer[ offset ] & 0xF );
				const auto	src = characterRom + ( character << 3 );

				auto	row = bitsToBytes[ src[ 0 ] ];			dst[ 0				  ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 1 ] ];			dst[ dstRowLength	  ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 2 ] ];			dst[ dstRowLength * 2 ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 3 ] ];			dst[ dstRowLength * 3 ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 4 ] ];			dst[ dstRowLength * 4 ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 5 ] ];			dst[ dstRowLength * 5 ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 6 ] ];			dst[ dstRowLength * 6 ] = ( row & color ) | ( ( ~row ) & bckCol );
						row = bitsToBytes[ src[ 7 ] ];			dst[ dstRowLength * 7 ] = ( row & color ) | ( ( ~row ) & bckCol );
			}

			++dst;
			++offset;
		}
		dst += rowSkip;
	}
}
//-----------------------------------------------------------------------------

void VIC2_Render::generateTextCRT ( const uint8_t bckColors, uint8_t textColor, const char* text )
{
	// High-res image
	indexBufferWidth = outerUnscaledWidth;

	screenCol = bckColors & 0xF;
	borderCol = bckColors >> 4;

	std::fill_n ( screenBuffer, textColumns * textRows, 32 );
	std::fill_n ( colorBuffer, textColumns * textRows, textColor );

	// Place text
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
		int		x = 0;
		int		y = 0;

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

	renderScreen ();
	backupIndexBuffer ();
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

void VIC2_Render::backupIndexBuffer ()
{
	if ( indexBufferBackup.empty () )
		return;

	auto	src = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::readOnly ).data;

	std::copy_n ( src, outerUnscaledLength, indexBufferBackup.data () );
}
//-----------------------------------------------------------------------------

void VIC2_Render::restoreIndexBuffer ()
{
	if ( indexBufferBackup.empty () )
		return;

	auto	dst = (uint8_t*)juce::Image::BitmapData ( indexBuffer, juce::Image::BitmapData::ReadWriteMode::writeOnly ).data;

	std::copy_n ( indexBufferBackup.data (), outerUnscaledLength, dst);
}
//-----------------------------------------------------------------------------

juce::Image VIC2_Render::getThumbnail ()
{
	// Render as CRT image
	renderCRT ();

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
}
//-----------------------------------------------------------------------------
