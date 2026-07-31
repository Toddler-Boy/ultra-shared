#include <JuceHeader.h>

#if ! JUCE_MAC

#include "GUI_FatalError.h"

//-----------------------------------------------------------------------------

static constexpr unsigned char fatalErrorPNG[] =
{
	#embed "GUI_FatalError.png"
};
//-----------------------------------------------------------------------------

GUI_FatalError::GUI_FatalError ()
{
	setName ( "fatal-error" );
	setOpaque ( true );
}
//-----------------------------------------------------------------------------

void GUI_FatalError::paint ( juce::Graphics& g )
{
	if ( bitmap.isNull () )
	{
		bitmap = juce::ImageFileFormat::loadFrom ( fatalErrorPNG, int ( std::size ( fatalErrorPNG ) ) );
		bgCol = bitmap.getPixelAt ( 0, 0 );
	}

	g.fillAll ( bgCol );

	g.setImageResamplingQuality ( juce::Graphics::highResamplingQuality );
	g.drawImage ( bitmap, getLocalBounds ().toFloat (), 0 );
}
//-----------------------------------------------------------------------------

#endif
