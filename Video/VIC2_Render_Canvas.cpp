#include <JuceHeader.h>

#include "VIC2_Render.h"

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

void VIC2_Render::storeCanvas ( const char* filename, std::vector<uint8_t> indices, const int width, const int height )
{
	numFields = 1;
	curField = 0;

	canvas = std::move ( indices );
	canvasWidth = width;
	canvasHeight = height;
	scrollX = 0;
	scrollY = 0;

	multicolor = true;
	for ( auto y = 0; y < height && multicolor; ++y )
	{
		const auto*	row = canvas.data () + size_t ( y ) * size_t ( width );

		for ( auto x = 0; x + 1 < width; x += 2 )
			if ( row[ x ] != row[ x + 1 ] )
			{
				multicolor = false;
				break;
			}
	}

	// The picture replaced whatever renderScreen drew; the border comes from the name
	invalidate ();
	indexBufferWidth = innerUnscaledWidth;

	findBorderColor ( filename );
	restoreIndexBuffer ();
}
//-----------------------------------------------------------------------------

void VIC2_Render::clearCanvas ()
{
	canvas.clear ();
	canvasWidth = 0;
	canvasHeight = 0;
	scrollX = 0;
	scrollY = 0;
}
//-----------------------------------------------------------------------------

void VIC2_Render::setScroll ( const int x, const int y )
{
	scrollX = std::clamp ( x, 0, getScrollRangeX () );
	scrollY = std::clamp ( y, 0, getScrollRangeY () );
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
	if ( ! canvas.empty () )
	{
		fillBorder ();

		for ( auto y = 0; y < innerUnscaledHeight; ++y )
			std::copy_n ( canvas.data () + size_t ( y + scrollY ) * size_t ( canvasWidth ) + size_t ( scrollX ), innerUnscaledWidth,
						  indexPixels + ( y + unscaledBorderSizeY ) * outerUnscaledWidth + unscaledBorderSizeX );
		return;
	}

	if ( indexBufferBackup.empty () )
		return;

	std::copy_n ( indexBufferBackup.data () + size_t ( curField ) * outerUnscaledLength, outerUnscaledLength, indexPixels );
}
//-----------------------------------------------------------------------------
