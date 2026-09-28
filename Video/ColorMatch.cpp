#include <JuceHeader.h>

#include "ColorMatch.h"

//-----------------------------------------------------------------------------

std::vector<uint8_t> colormatch::matchToVIC2 ( const colodore& colo, const std::vector<uint32_t>& imgPalette, const colodore::rgbPalette& refPalette )
{
	// No C64 palette has two entries this close per channel, so they are one color
	constexpr auto	sameColorSpread = 8;

	// Below this chroma a source color is a grey and only takes black, white or a grey
	constexpr auto	greyChroma = 8.0f;

	// One hue sector off costs as much as four red to light-red luma steps
	constexpr auto	hueSector = 360.0f / 16.0f;
	constexpr auto	hueWeight = 4.0f;
	constexpr auto	lumaStep = 48.0f;

	// A color may leave its nearest index by two sectors to stay distinct; beyond that it shares
	constexpr auto	costScale = 1000.0f;
	constexpr auto	maxDetour = int ( 16.0f * costScale );

	// Dwarfs any sum of in-budget costs, so fewer shares always win
	constexpr auto	sharePenalty = 1 << 24;

	auto sameColor = [] ( const uint32_t a, const uint32_t b )
	{
		for ( auto shift = 0; shift < 24; shift += 8 )
			if ( std::abs ( int ( ( a >> shift ) & 0xFF ) - int ( ( b >> shift ) & 0xFF ) ) > sameColorSpread )
				return false;
		return true;
	};

	// Near-duplicates collapse onto their first occurrence
	std::vector<uint32_t>	colors;
	std::vector<uint8_t>	colorPos ( imgPalette.size () );

	for ( size_t i = 0; i < imgPalette.size (); ++i )
	{
		const auto	col = imgPalette[ i ];
		const auto	same = std::ranges::find_if ( colors, [ & ] ( const uint32_t other ) { return sameColor ( col, other ); } );

		colorPos[ i ] = uint8_t ( same - colors.begin () );
		if ( same == colors.end () )
			colors.emplace_back ( col );
	}

	auto hueDiff = [] ( const float a, const float b )
	{
		const auto	d = std::fmod ( std::abs ( a - b ), 360.0f );
		return std::min ( d, 360.0f - d );
	};

	const auto	numColors = int ( colors.size () );
	const auto	numIndices = int ( refPalette.size () );

	std::array<std::array<int, 16>, 16>	cost {};
	for ( auto pos = 0; pos < numColors; ++pos )
	{
		const auto	src = colo.rgb2polar ( colors[ size_t ( pos ) ] );
		const auto	grey = src.chroma <= greyChroma;

		auto&	row = cost[ pos ];
		for ( auto index = 0; index < numIndices; ++index )
		{
			const auto	angle = colo.chromaAngle ( index );
			if ( grey == angle.has_value () )
			{
				row[ index ] = INT_MAX;
				continue;
			}

			const auto	luma = ( src.luma - colo.rgb2polar ( refPalette[ size_t ( index ) ] ).luma ) / lumaStep;
			const auto	hue = angle ? hueDiff ( src.hue, *angle ) / hueSector : 0.0f;

			row[ index ] = int ( ( luma * luma + hueWeight * hue * hue ) * costScale );
		}

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

	std::vector<uint8_t>	chosen ( colors.size () );
	for ( auto pos = numColors - 1; pos >= 0; --pos )
	{
		const auto	taken = step[ size_t ( pos ) * numMasks + mask ];
		const auto	index = uint8_t ( taken & ~sharedBit );

		chosen[ size_t ( pos ) ] = index;

		if ( ! ( taken & sharedBit ) )
			mask &= ~( size_t ( 1 ) << index );
	}

	std::vector<uint8_t>	out ( imgPalette.size () );
	for ( size_t i = 0; i < out.size (); ++i )
		out[ i ] = chosen[ colorPos[ i ] ];

	return out;
}
//-----------------------------------------------------------------------------
