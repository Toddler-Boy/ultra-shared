#include <JuceHeader.h>

#include <array>
#include <csetjmp>
#include <cstring>

#if ! defined (JUCE_INCLUDE_PNGLIB_CODE) || JUCE_INCLUDE_PNGLIB_CODE
#include "juce_graphics/image_formats/pnglib/png.h"
#else
extern "C"
{
#include JUCE_PNGLIB_INCLUDE_PATH
}
#endif

#include "PNG_Loader.h"

//-----------------------------------------------------------------------------

namespace
{
	struct readState
	{
		const uint8_t*	data;
		size_t			size;
		size_t			pos;
	};

	void readCallback ( png_structp png, png_bytep dst, png_size_t length )
	{
		auto&	state = *static_cast<readState*> ( png_get_io_ptr ( png ) );

		if ( length > state.size - state.pos )
			png_error ( png, "" );	// Does not return

		std::memcpy ( dst, state.data + state.pos, length );
		state.pos += length;
	}

	void errorCallback ( png_structp png, png_const_charp )
	{
		longjmp ( png_jmpbuf ( png ), 1 );
	}

	void warningCallback ( png_structp, png_const_charp ) {}

	void writeCallback ( png_structp png, png_bytep data, png_size_t length )
	{
		auto&	out = *static_cast<std::vector<uint8_t>*> ( png_get_io_ptr ( png ) );
		out.insert ( out.end (), data, data + length );
	}

	void flushCallback ( png_structp ) {}
}
//-----------------------------------------------------------------------------

pngloader::image pngloader::decode ( const void* data, const size_t size )
{
	image	out;

	if ( size < 8 || png_sig_cmp ( (png_const_bytep)data, 0, 8 ) != 0 )
		return out;

	auto	png = png_create_read_struct ( PNG_LIBPNG_VER_STRING, nullptr, errorCallback, warningCallback );
	if ( png == nullptr )
		return out;

	auto	info = png_create_info_struct ( png );
	if ( info == nullptr )
	{
		png_destroy_read_struct ( &png, nullptr, nullptr );
		return out;
	}

	readState				state { (const uint8_t*)data, size, 0 };
	std::vector<png_bytep>	rows;

	volatile auto	ok = false;

	// libpng reports errors via longjmp, landing back here with a nonzero code
	if ( setjmp ( png_jmpbuf ( png ) ) == 0 )
	{
		png_set_read_fn ( png, &state, readCallback );

		// Keeps a corrupt header from requesting a giant allocation
		png_set_user_limits ( png, 8192, 8192 );

		png_read_info ( png, info );

		png_uint_32	w = 0;
		png_uint_32	h = 0;
		auto		bitDepth = 0;
		auto		colorType = 0;
		png_get_IHDR ( png, info, &w, &h, &bitDepth, &colorType, nullptr, nullptr, nullptr );

		const auto	paletted = colorType == PNG_COLOR_TYPE_PALETTE;

		if ( paletted )
		{
			// Keep the indices, one byte each regardless of stored bit depth
			if ( bitDepth < 8 )
				png_set_packing ( png );
		}
		else
		{
			// Everything else becomes 8-bit BGRA (0xAARRGGBB in memory)
			if ( bitDepth == 16 )
				png_set_strip_16 ( png );

			if ( bitDepth < 8 || png_get_valid ( png, info, PNG_INFO_tRNS ) )
				png_set_expand ( png );

			if ( colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA )
				png_set_gray_to_rgb ( png );

			png_set_bgr ( png );
			png_set_add_alpha ( png, 0xFF, PNG_FILLER_AFTER );
		}

		png_set_interlace_handling ( png );
		png_read_update_info ( png, info );

		rows.resize ( h );

		if ( paletted )
		{
			out.indices.resize ( size_t ( w ) * h );
			for ( auto y = 0u; y < h; ++y )
				rows[ y ] = out.indices.data () + size_t ( y ) * w;
		}
		else
		{
			out.pixels.resize ( size_t ( w ) * h );
			for ( auto y = 0u; y < h; ++y )
				rows[ y ] = (png_bytep)( out.pixels.data () + size_t ( y ) * w );
		}

		png_read_image ( png, rows.data () );

		if ( paletted )
		{
			png_colorp	pal = nullptr;
			auto		palSize = 0;
			png_get_PLTE ( png, info, &pal, &palSize );

			out.palette.resize ( size_t ( palSize ) );
			for ( auto i = 0; i < palSize; ++i )
				out.palette[ i ] = uint32_t ( pal[ i ].red ) << 16 | uint32_t ( pal[ i ].green ) << 8 | pal[ i ].blue;

			out.paletted = true;
		}

		out.width = int ( w );
		out.height = int ( h );

		ok = true;
	}

	png_destroy_read_struct ( &png, &info, nullptr );

	if ( ! ok )
		return {};

	return out;
}
//-----------------------------------------------------------------------------

std::vector<uint8_t> pngloader::encode ( const image& img )
{
	if ( ! img.isValid () )
		return {};

	auto	png = png_create_write_struct ( PNG_LIBPNG_VER_STRING, nullptr, errorCallback, warningCallback );
	if ( png == nullptr )
		return {};

	auto	info = png_create_info_struct ( png );
	if ( info == nullptr )
	{
		png_destroy_write_struct ( &png, nullptr );
		return {};
	}

	std::vector<uint8_t>		out;
	std::vector<png_bytep>		rows ( size_t ( img.height ) );
	std::array<png_color, 256>	pal {};

	volatile auto	ok = false;

	// libpng reports errors via longjmp, landing back here with a nonzero code
	if ( setjmp ( png_jmpbuf ( png ) ) == 0 )
	{
		png_set_write_fn ( png, &out, writeCallback, flushCallback );

		png_set_IHDR ( png, info, png_uint_32 ( img.width ), png_uint_32 ( img.height ), 8,
					   img.paletted ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB_ALPHA,
					   PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

		if ( img.paletted )
		{
			for ( size_t i = 0; i < img.palette.size (); ++i )
			{
				pal[ i ].red	= uint8_t ( img.palette[ i ] >> 16 );
				pal[ i ].green	= uint8_t ( img.palette[ i ] >> 8 );
				pal[ i ].blue	= uint8_t ( img.palette[ i ] );
			}
			png_set_PLTE ( png, info, pal.data (), int ( img.palette.size () ) );
		}

		png_write_info ( png, info );

		if ( img.paletted )
		{
			for ( auto y = 0; y < img.height; ++y )
				rows[ y ] = (png_bytep)( img.indices.data () + size_t ( y ) * img.width );
		}
		else
		{
			// Pixels are 0xAARRGGBB in memory
			png_set_bgr ( png );

			for ( auto y = 0; y < img.height; ++y )
				rows[ y ] = (png_bytep)( img.pixels.data () + size_t ( y ) * img.width );
		}

		png_write_image ( png, rows.data () );
		png_write_end ( png, info );

		ok = true;
	}

	png_destroy_write_struct ( &png, &info );

	if ( ! ok )
		return {};

	return out;
}
//-----------------------------------------------------------------------------
