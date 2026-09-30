#include <algorithm>
#include <climits>
#include <cstdlib>

#include "PictureAnalyzer.h"

//-----------------------------------------------------------------------------

namespace
{
	int countBadPairs ( const pictureanalyzer::picture& pic, const int phase )
	{
		auto	count = 0;

		for ( auto y = 0; y < pic.height; ++y )
		{
			const auto*	row = pic.pixels + size_t ( y ) * size_t ( pic.stride );

			for ( auto x = phase; x + 1 < pic.width; x += 2 )
				count += row[ x ] != row[ x + 1 ];
		}

		return count;
	}

	// Exports place the screen a few lines off. A line with a colour change off the 8 pixel
	// grid between the screen's columns holds graphics; raster splits and plain lines do
	// not. When all such lines fit one screen it sits over them, nearest when that leaves
	// room; graphics above or below it (sprites) leave everything where it is
	int screenTop ( const pictureanalyzer::picture& pic, const int frameHeight )
	{
		constexpr auto	maxShift = 8;

		const auto	offset = int ( pic.pixels - pic.frame );
		const auto	borderX = offset % pic.stride;
		const auto	borderY = offset / pic.stride;

		auto	minY = INT_MAX;
		auto	maxY = -1;

		for ( auto y = 0; y < frameHeight; ++y )
		{
			const auto*	row = pic.frame + size_t ( y ) * size_t ( pic.stride );

			for ( auto x = borderX + 1; x < borderX + pic.width; ++x )
			{
				if ( row[ x ] != row[ x - 1 ] && ( ( x - borderX ) & 7 ) != 0 )
				{
					minY = std::min ( minY, y );
					maxY = y;
					break;
				}
			}
		}

		const auto	low = std::max ( 0, maxY - ( pic.height - 1 ) );
		const auto	high = std::min ( minY, frameHeight - pic.height );

		if ( maxY < 0 || low > high )
			return borderY;

		// Exports are off by a few lines; farther means art in one border only
		const auto	top = std::clamp ( borderY, low, high );
		return std::abs ( top - borderY ) <= maxShift ? top : borderY;
	}

	// With both frame edges one colour on every screen line (plain sides, sprites at most
	// above and below), the window sits over the picture's content, nearest when that
	// leaves room; side graphics keep the caller's column
	int screenLeft ( const pictureanalyzer::picture& pic, const int top )
	{
		const auto	borderX = int ( pic.pixels - pic.frame ) % pic.stride;
		const auto	frameWidth = pic.stride;

		auto	minX = INT_MAX;
		auto	maxX = -1;

		for ( auto y = top; y < top + pic.height; ++y )
		{
			const auto*	row = pic.frame + size_t ( y ) * size_t ( frameWidth );
			const auto	side = row[ 0 ];

			if ( row[ frameWidth - 1 ] != side )
				return borderX;

			for ( auto x = 0; x < frameWidth; ++x )
			{
				if ( row[ x ] != side )
				{
					minX = std::min ( minX, x );
					maxX = std::max ( maxX, x );
				}
			}
		}

		const auto	low = std::max ( 0, maxX - ( pic.width - 1 ) );
		const auto	high = std::min ( minX, frameWidth - pic.width );

		if ( maxX < 0 || low > high )
			return borderX;

		return std::clamp ( borderX, low, high );
	}

	// The caller's screen position has equal borders above and below
	int frameHeightOf ( const pictureanalyzer::picture& pic )
	{
		return pic.height + int ( pic.pixels - pic.frame ) / pic.stride * 2;
	}

	// Border colour changes happen on CPU cycles, 8 pixels on the screen's grid, or from
	// one raster line to the next; any other change in the border is a sprite
	uint16_t borderFlags ( const pictureanalyzer::picture& pic, const int frameHeight, const int ox )
	{
		const auto	offset = int ( pic.pixels - pic.frame );
		const auto	borderX = offset % pic.stride;
		const auto	borderY = offset / pic.stride;
		const auto	frameWidth = pic.stride;

		auto	flags = uint16_t ( 0 );

		auto	previousEdge = -1;
		auto	sideSprites = false;
		auto	sideArt = false;

		auto scan = [ & ] ( const uint8_t* row, const int from, const int to, bool& onGrid, bool& offGrid )
		{
			for ( auto x = from + 1; x < to; ++x )
				if ( row[ x ] != row[ x - 1 ] )
					( ( ( x - borderX - ox ) & 7 ) != 0 ? offGrid : onGrid ) = true;
		};

		for ( auto y = 0; y < frameHeight; ++y )
		{
			const auto*	row = pic.frame + size_t ( y ) * size_t ( frameWidth );

			auto	onGrid = false;
			auto	offGrid = false;

			// Above and below the screen only sprites count: an opened border, a colour
			// switch where the screen starts and sprite edges all land on the grid there
			if ( y < borderY || y >= borderY + pic.height )
			{
				scan ( row, 0, frameWidth, onGrid, offGrid );

				if ( offGrid )
					flags |= pictureanalyzer::borderSprites;

				// A plain line there is border colour too, a split where the screen starts shows only here
				if ( ! onGrid && ! offGrid )
				{
					if ( previousEdge >= 0 && row[ 0 ] != previousEdge )
						flags |= pictureanalyzer::rasterSplits;

					previousEdge = row[ 0 ];
				}

				continue;
			}

			scan ( row, 0, borderX, onGrid, offGrid );
			scan ( row, frameWidth - borderX, frameWidth, onGrid, offGrid );

			if ( offGrid )
			{
				flags |= pictureanalyzer::borderSprites;
				sideSprites = true;
			}

			// A raster split recolours the whole line: both side borders one and the same
			// colour, different from the last such line
			const auto*	right = row + frameWidth - borderX;
			const auto	plain = std::all_of ( row, row + borderX, [ row ] ( const uint8_t p ) { return p == row[ 0 ]; } )
							&& std::all_of ( right, right + borderX, [ row ] ( const uint8_t p ) { return p == row[ 0 ]; } );

			if ( plain )
			{
				if ( previousEdge >= 0 && row[ 0 ] != previousEdge )
					flags |= pictureanalyzer::rasterSplits;

				previousEdge = row[ 0 ];
			}
			else
				sideArt = true;
		}

		// Side borders not one colour on both sides of every line hold artwork, sprites in
		// practice; their colour changes are no raster splits
		if ( sideArt )
			flags |= pictureanalyzer::borderSprites;

		if ( sideSprites || sideArt )
			flags &= uint16_t ( ~pictureanalyzer::rasterSplits );

		return flags;
	}
}
//-----------------------------------------------------------------------------

pictureanalyzer::position pictureanalyzer::screenPosition ( const picture& pic )
{
	const auto	top = screenTop ( pic, frameHeightOf ( pic ) );

	return { screenLeft ( pic, top ), top };
}
//-----------------------------------------------------------------------------

uint16_t pictureanalyzer::analyze ( const picture& input )
{
	const auto	frameHeight = input.frame ? frameHeightOf ( input ) : 0;

	auto	pic = input;
	if ( pic.frame )
	{
		const auto	borderX = int ( input.pixels - input.frame ) % input.stride;
		pic.pixels = pic.frame + size_t ( screenTop ( input, frameHeight ) ) * size_t ( pic.stride ) + size_t ( borderX );
	}

	// Wide-pixel art keeps its pixel pairs in one phase; a few stray single-width pixels
	// do not make it hires, hires art breaks thousands of pairs
	const auto	badEven = countBadPairs ( pic, 0 );
	const auto	badOdd = countBadPairs ( pic, 1 );
	const auto	multi = std::min ( badEven, badOdd ) * 100 < pic.width / 2 * pic.height;

	auto	flags = uint16_t ( multi ? multicolor : 0 );

	if ( pic.frame )
	{
		const auto	ox = multi && badOdd < badEven ? 1 : 0;

		flags |= borderFlags ( pic, frameHeight, ox );
	}

	return flags;
}
//-----------------------------------------------------------------------------

uint16_t pictureanalyzer::combine ( const uint16_t a, const uint16_t b )
{
	return uint16_t ( ( a & b & multicolor ) | ( ( a | b ) & ( borderSprites | rasterSplits ) ) );
}
//-----------------------------------------------------------------------------
