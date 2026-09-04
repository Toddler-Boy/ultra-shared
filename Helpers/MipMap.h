#pragma once

#include <JuceHeader.h>

// Post-processing of the reduced levels, off by default. 1.0 is the strength tuned for
// the VIC2 thumbnails, higher values are allowed. Lives outside the class so it is
// complete when the default arguments below need it
struct MipMapEnhance
{
	float	sharpen = 0.0f;
	float	saturation = 0.0f;
};
//-------------------------------------------------------------------------------------------------

class MipMap final
{
public:
	using Enhance = MipMapEnhance;

	MipMap () = default;
	MipMap ( const juce::Image& src, const Enhance& enhance = {} )	{	setImage ( src, enhance );	}

	void setImage ( const juce::Image& src, const Enhance& enhance = {} );
	void setImage ( const juce::File& f, const Enhance& enhance = {} );
	void setImage ( const void* rawData, size_t numBytesOfData, const Enhance& enhance = {} );

	void draw ( juce::Graphics& g, juce::Rectangle<float> rc, juce::RectanglePlacement placement = 0 );

	[[ nodiscard ]] juce::Image getImageFor ( const int width, const int height );
	[[ nodiscard ]] juce::Image getImage () { return images[ 0 ]; }

	[[ nodiscard ]] juce::Rectangle<int> getBounds ();

	[[ nodiscard ]] int numMipMaps () const		{	return int ( images.size () );	}
	[[ nodiscard ]] bool isValid () const		{	return numMipMaps ();			}
	[[ nodiscard ]] bool isNull () const		{	return ! isValid ();			}
	[[ nodiscard ]] int getWidth () const		{	return isValid () ? images[ 0 ].getWidth () : 0; }
	[[ nodiscard ]] int getHeight () const		{	return isValid () ? images[ 0 ].getHeight () : 0; }

	[[ nodiscard ]] int getNumBytesOfData () const;
	[[ nodiscard ]] const Enhance& getEnhance () const	{	return enhance;	}

private:
	[[ nodiscard ]] int getIndexFor ( int width, int height ) const;

	std::vector<juce::Image>	images;
	Enhance						enhance;
};
//-------------------------------------------------------------------------------------------------
