#include <JuceHeader.h>

#include <bit>

#include "MipMap.h"
//-----------------------------------------------------------------------------

template <class T>
static juce::Image reduceImageByHalf ( const juce::Image& src )
{
	const auto	w = src.getWidth () / 2;
	const auto	h = src.getHeight () / 2;

	juce::Image dst ( src.getFormat (), w, h, false );

	const juce::Image::BitmapData srcData ( src, juce::Image::BitmapData::readOnly );
	const juce::Image::BitmapData dstData ( dst, juce::Image::BitmapData::readWrite );

	for ( auto y = 0; y < h; ++y )
	{
		auto	srcLine1 = srcData.getLinePointer ( y << 1 );
		auto	srcLine2 = srcData.getLinePointer ( ( y << 1 ) + 1 );
		auto	dstLine = dstData.getLinePointer ( y );

		for ( auto x = 0; x < w; ++x )
		{
			auto	a = 0;
			auto	r = 0;
			auto	g = 0;
			auto	b = 0;

			auto incColors = [ &a, &r, &g, &b ] ( auto* d )
			{
				a += d->getAlpha ();
				r += d->getRed ();
				g += d->getGreen ();
				b += d->getBlue ();
			};

			incColors ( (T*)srcLine1 );	srcLine1 += srcData.pixelStride;
			incColors ( (T*)srcLine1 );	srcLine1 += srcData.pixelStride;
			incColors ( (T*)srcLine2 );	srcLine2 += srcData.pixelStride;
			incColors ( (T*)srcLine2 );	srcLine2 += srcData.pixelStride;

			// Rounded, so the levels do not drift darker step by step
			((T*)dstLine)->setARGB ( uint8_t ( ( a + 2 ) >> 2 ), uint8_t ( ( r + 2 ) >> 2 ), uint8_t ( ( g + 2 ) >> 2 ), uint8_t ( ( b + 2 ) >> 2 ) );

			dstLine += dstData.pixelStride;
		}
	}

	return dst;
}
//-------------------------------------------------------------------------------------------------

static juce::Image reduceImageByHalf ( const juce::Image& src )
{
	if ( src.getFormat () == juce::Image::ARGB )
		return reduceImageByHalf<juce::PixelARGB> ( src );

	if ( src.getFormat () == juce::Image::RGB )
		return reduceImageByHalf<juce::PixelRGB> ( src );

	jassertfalse;
	return {};
}
//-------------------------------------------------------------------------------------------------

// Heavy reductions average dithered and clashing colours to a mid-tone mean that reads
// as grey at thumbnail size, so every level from the second halving on gets a small
// saturation lift, applied once to the stored copy so it never compounds. gin scales
// the part above 100 by three, hence the division. Both this and the sharpening below
// are scaled by the caller's Enhance: 0 leaves the levels alone, 1 gives these values.
static constexpr float	saturationPerLevel = 15.0f;	// percent
static constexpr float	saturationMax = 40.0f;

// The box filter softens a little on every step, and by 128 px and below that reads as
// blur. gin's sharpen is a fixed kernel, so the strength comes from blending the
// sharpened copy back over the level; a strength beyond one full pass sharpens again on top.
static constexpr int	sharpenBelow = 128;
static constexpr float	sharpenAmount = 0.6f;

void MipMap::setImage ( const juce::Image& src, const Enhance& newEnhance )
{
	images.clear ();
	enhance = newEnhance;
	if ( src.isNull () )
		return;

	auto	level = juce::NativeImageType ().convert ( src );
	images.emplace_back ( level );

	while ( level.getWidth () > 16 && level.getHeight () > 16 )
	{
		level = reduceImageByHalf ( level );

		const auto	lift = std::min ( saturationMax, saturationPerLevel * float ( images.size () - 1 ) ) * enhance.saturation;
		auto		sharpen = std::max ( level.getWidth (), level.getHeight () ) <= sharpenBelow ? sharpenAmount * enhance.sharpen : 0.0f;

		if ( lift <= 0.0f && sharpen <= 0.0f )
		{
			images.emplace_back ( level );
			continue;
		}

		// The untouched chain feeds the next level; only the stored copy gets the mask and the lift
		auto	stored = level.createCopy ();

		for ( ; sharpen > 0.0f; sharpen -= 1.0f )
		{
			auto	sharp = stored.createCopy ();
			gin::applySharpen ( sharp );
			gin::applyBlend ( stored, sharp, gin::BlendMode::Normal, std::min ( sharpen, 1.0f ) );
		}

		if ( lift > 0.0f )
			gin::applyHueSaturationLightness ( stored, 0.0f, 100.0f + lift / 3.0f, 0.0f );

		images.emplace_back ( stored );
	}
}
//-------------------------------------------------------------------------------------------------

void MipMap::setImage ( const juce::File& f, const Enhance& newEnhance )
{
	setImage ( juce::ImageFileFormat::loadFrom ( f ), newEnhance );
}
//-------------------------------------------------------------------------------------------------

void MipMap::setImage ( const void* rawData, size_t numBytesOfData, const Enhance& newEnhance )
{
	setImage ( juce::ImageFileFormat::loadFrom ( rawData, numBytesOfData ), newEnhance );
}
//-------------------------------------------------------------------------------------------------

void MipMap::draw ( juce::Graphics& g, juce::Rectangle<float> rc, juce::RectanglePlacement placement )
{
	if ( images.empty () || images[ 0 ].isNull () )
		return;

	const auto	scale = g.getInternalContext ().getPhysicalPixelScaleFactor ();
	const auto	mipRect = ( rc * scale ).getSmallestIntegerContainer ();

	const auto&	img = images[ size_t ( getIndexFor ( mipRect.getWidth (), mipRect.getHeight () ) ) ];

	auto	quality =	( img.getWidth () == mipRect.getWidth () && img.getHeight () == mipRect.getHeight () ) ?
							juce::Graphics::lowResamplingQuality
							:
							juce::Graphics::highResamplingQuality;

	g.setImageResamplingQuality ( quality );
	g.drawImage ( img, rc, placement );
}
//-------------------------------------------------------------------------------------------------

juce::Image MipMap::getImageFor ( const int width, const int height )
{
	if ( images.empty () )
		return {};

	return images[ size_t ( getIndexFor ( width, height ) ) ];
}
//-------------------------------------------------------------------------------------------------

int MipMap::getIndexFor ( const int width, const int height ) const
{
	if ( width <= 0 || height <= 0 )
		return int ( images.size () ) - 1;

	// level k is floor ( source / 2^k ), so the wanted level is floor ( log2 ( min ( ratios ) ) )
	const auto	ratio = std::min ( images[ 0 ].getWidth () / width, images[ 0 ].getHeight () / height );
	if ( ratio < 1 )
		return 0;

	return std::min ( std::bit_width ( unsigned ( ratio ) ) - 1, int ( images.size () ) - 1 );
}
//-------------------------------------------------------------------------------------------------

juce::Rectangle<int> MipMap::getBounds ()
{
	if ( ! images.empty () )
		return images[ 0 ].getBounds ();

	return juce::Rectangle<int> ();
}
//-------------------------------------------------------------------------------------------------

int MipMap::getNumBytesOfData () const
{
	auto	numBytes = 0;

	for ( auto& i : images )
		numBytes += i.getWidth () * i.getHeight () * ( i.getFormat () == juce::Image::ARGB ? 4 : 3 );

	return numBytes;
}
//-------------------------------------------------------------------------------------------------
