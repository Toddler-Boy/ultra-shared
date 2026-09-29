#include <JuceHeader.h>

#include "ultra-shared/Helpers/ImageUtils.h"

#include "ColorMatch.h"
#include "PNG_Loader.h"
#include "VIC2_Render.h"

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
	clearCanvas ();

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

// Pictures a PNG stacks: 1 at 320x200 or 384x272, 2 interlace fields at doubled height
// with the interlace hint, 0 = a larger borderless picture area, -1 = unusable
static int fieldsForSize ( const char* filename, const int width, const int height )
{
	const auto	frameHeight =	width == VIC2_Render::innerUnscaledWidth ? VIC2_Render::innerUnscaledHeight
							:	width == VIC2_Render::outerUnscaledWidth ? VIC2_Render::outerUnscaledHeight : 0;

	if ( imageutils::hintFromFilename ( filename ).interlaced )
		return frameHeight != 0 && height == frameHeight * 2 ? 2 : -1;

	if ( frameHeight != 0 && height == frameHeight )
		return 1;

	return width >= VIC2_Render::innerUnscaledWidth && height >= VIC2_Render::innerUnscaledHeight ? 0 : -1;
}
//-----------------------------------------------------------------------------

template <typename T, typename F>
static std::vector<uint8_t> toIndices ( const T* src, const int width, const int height, F convert )
{
	std::vector<uint8_t>	indices ( size_t ( width ) * size_t ( height ) );
	std::transform ( src, src + indices.size (), indices.begin (), convert );

	return indices;
}
//-----------------------------------------------------------------------------

template <typename T, typename F>
void VIC2_Render::storeFields ( const char* filename, const T* src, const int width, const int fields, F convert )
{
	const auto	inner = width == innerUnscaledWidth;
	const auto	height = inner ? innerUnscaledHeight : outerUnscaledHeight;
	const auto	rowByteSkip = inner ? unscaledBorderSizeX * 2 : 0;

	numFields = fields;

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

	const auto	fields = fieldsForSize ( filename, width, height );
	if ( fields < 0 )
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
	const auto	vic2Indices = colormatch::matchToVIC2 ( colo, imagePalette, referencePalette );

	std::unordered_map<uint32_t, uint8_t>	mapped;
	for ( size_t i = 0; i < imagePalette.size (); ++i )
		mapped[ imagePalette[ i ] ] = vic2Indices[ i ];

	//
	// Third pass: create index buffer
	//
	const auto	convert = [ &mapped ] ( const uint32_t col ) { return mapped[ col ]; };

	if ( fields == 0 )
		storeCanvas ( filename, toIndices ( rawData, width, height, convert ), width, height );
	else
		storeFields ( filename, rawData, width, fields, convert );

	return true;
}
//-----------------------------------------------------------------------------

bool VIC2_Render::convertPaletted ( const char* filename, const pngloader::image& img )
{
	indexBufferWidth = 0;

	const auto	fields = fieldsForSize ( filename, img.width, img.height );
	if ( fields < 0 )
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

	const auto	vic2Indices = colormatch::matchToVIC2 ( colo, colors, referencePalette );

	// Palette-index to vic2-index blit table
	std::array<uint8_t, 256>	lut {};
	for ( auto i = 0; i < int ( img.palette.size () ); ++i )
		if ( used[ i ] )
			lut[ i ] = vic2Indices[ colorPos[ i ] ];

	const auto	convert = [ &lut ] ( const uint8_t index ) { return lut[ index ]; };

	if ( fields == 0 )
		storeCanvas ( filename, toIndices ( img.indices.data (), img.width, img.height, convert ), img.width, img.height );
	else
		storeFields ( filename, img.indices.data (), img.width, fields, convert );

	return true;
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

uint16_t VIC2_Render::analyze () const
{
	if ( ! canvas.empty () )
		return pictureanalyzer::analyze ( { canvas.data (), canvasWidth, canvasHeight, canvasWidth, nullptr } );

	auto field = [ & ] ( const uint8_t* frame )
	{
		return pictureanalyzer::analyze ( { frame + unscaledBorderSizeY * outerUnscaledWidth + unscaledBorderSizeX, innerUnscaledWidth, innerUnscaledHeight,
											outerUnscaledWidth, indexBufferWidth == outerUnscaledWidth ? frame : nullptr } );
	};

	// A second field lives in the backup only
	if ( numFields == 1 )
		return field ( indexPixels );

	return pictureanalyzer::combine ( field ( indexBufferBackup.data () ), field ( indexBufferBackup.data () + outerUnscaledLength ) );
}
//-----------------------------------------------------------------------------
