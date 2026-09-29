#include <algorithm>
#include <climits>
#include <vector>

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

	// Exports place the screen a few lines off: it sits where the fewest border changes off
	// the 8 pixel grid are left (picture detail outside it makes them), nearest on a tie
	int screenTop ( const pictureanalyzer::picture& pic, const int frameHeight )
	{
		constexpr auto	maxShift = 8;

		const auto	offset = int ( pic.pixels - pic.frame );
		const auto	borderX = offset % pic.stride;
		const auto	borderY = offset / pic.stride;
		const auto	frameWidth = pic.stride;

		std::vector<int>	lineChanges ( size_t ( frameHeight ), 0 );
		std::vector<int>	sideChanges ( size_t ( frameHeight ), 0 );

		for ( auto y = 0; y < frameHeight; ++y )
		{
			const auto*	row = pic.frame + size_t ( y ) * size_t ( frameWidth );

			for ( auto x = 1; x < frameWidth; ++x )
			{
				if ( row[ x ] == row[ x - 1 ] || ( ( x - borderX ) & 7 ) == 0 )
					continue;

				++lineChanges[ size_t ( y ) ];

				if ( x < borderX || x > frameWidth - borderX )
					++sideChanges[ size_t ( y ) ];
			}
		}

		auto	best = borderY;
		auto	fewest = INT_MAX;

		for ( auto shift = 0; shift <= maxShift; ++shift )
		{
			for ( const auto top : { borderY - shift, borderY + shift } )
			{
				if ( top < 0 || top + pic.height > frameHeight )
					continue;

				auto	changes = 0;
				for ( auto y = 0; y < frameHeight; ++y )
					changes += y >= top && y < top + pic.height ? sideChanges[ size_t ( y ) ] : lineChanges[ size_t ( y ) ];

				if ( changes < fewest )
				{
					fewest = changes;
					best = top;
				}
			}
		}

		return best;
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
		}

		// Sprites beside the screen mean opened side borders, which show the background
		// colour there; that looks like a raster split but is not one
		if ( sideSprites )
			flags &= uint16_t ( ~pictureanalyzer::rasterSplits );

		return flags;
	}
}
//-----------------------------------------------------------------------------

uint16_t pictureanalyzer::analyze ( const picture& input )
{
	// The caller's screen position has equal borders above and below
	const auto	frameHeight = input.frame ? input.height + int ( input.pixels - input.frame ) / input.stride * 2 : 0;

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
