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

			((T*)dstLine)->setARGB ( uint8_t ( a >> 2 ), uint8_t ( r >> 2 ), uint8_t ( g >> 2 ), uint8_t ( b >> 2 ) );

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

void MipMap::setImage ( const juce::Image& src )
{
	images.clear ();
	if ( src.isNull () )
		return;

	auto	halfImage = juce::NativeImageType ().convert ( src );
	images.emplace_back ( halfImage );

	while ( halfImage.getWidth () > 16 && halfImage.getHeight () > 16 )
	{
		halfImage = reduceImageByHalf ( halfImage );
		images.emplace_back ( halfImage );
	}
}
//-------------------------------------------------------------------------------------------------

void MipMap::setImage ( const juce::File& f )
{
	setImage ( juce::ImageFileFormat::loadFrom ( f ) );
}
//-------------------------------------------------------------------------------------------------

void MipMap::setImage ( const void* rawData, size_t numBytesOfData )
{
	setImage ( juce::ImageFileFormat::loadFrom ( rawData, numBytesOfData ) );
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
