#include <JuceHeader.h>

#include <algorithm>

#include "Config/DataSource.h"
#include "UI_Helpers.h"

#include "ultra-shared/Resources/Theme.h"
#include "UI/GUI_LookAndFeel.h"

//-----------------------------------------------------------------------------

namespace UI
{
	juce::Colour	startColor;
	juce::Colour	endColor;
}
//-----------------------------------------------------------------------------

void UI::setShades ( const juce::Colour col1, const juce::Colour col2 ) noexcept
{
	startColor = col1;
	endColor = col2;
}
//-----------------------------------------------------------------------------

juce::Colour UI::getShade ( const float blend ) noexcept
{
	return startColor.interpolatedWith ( endColor, blend );
}
//-----------------------------------------------------------------------------

juce::Colour UI::getAverageColor ( const juce::Image& img, const float bright, const float satMul, const float satDiv )
{
	auto	r = 0.0f;
	auto	g = 0.0f;
	auto	b = 0.0f;

	// Get average color, weighted by saturation
	{
		auto	src = juce::Image::BitmapData ( img, juce::Image::BitmapData::ReadWriteMode::readOnly );
		auto	countedPixels = 0.0f;
		auto	graySum = 0.0f;

		for ( auto y = 0; y < img.getHeight (); ++y )
		{
			for ( auto x = 0; x < img.getWidth (); ++x )
			{
				const auto	col = src.getPixelColour ( x, y ).withMultipliedSaturation ( satMul );
				const auto	sat = col.getSaturation ();

				r += col.getFloatRed () * sat;
				g += col.getFloatGreen () * sat;
				b += col.getFloatBlue () * sat;

				countedPixels += sat;
				graySum += col.getBrightness ();
			}
		}

		if ( countedPixels > 0.0f )
		{
			r /= countedPixels;
			g /= countedPixels;
			b /= countedPixels;
		}
		else
		{
			// Grayscale image, no saturation to weight by: average brightness
			r = g = b = graySum / float ( std::max ( 1, img.getWidth () * img.getHeight () ) );
		}
	}

	auto	col = juce::Colour::fromFloatRGBA ( r, g, b, 1.0f ).withMultipliedSaturation ( satDiv );

	if ( bright <= 0.0f )
		return col;

	return UI::getColorWithPerceivedBrightness ( col, bright );
}
//-----------------------------------------------------------------------------

juce::Colour UI::getColorFromName ( const juce::String& name, const float brightness )
{
	auto	hash = float ( double ( name.hashCode64 () ) / double ( std::numeric_limits<int64_t>::max () ) );

	return juce::Colour::fromHSV ( hash, 0.5f, brightness, 1.0f );
}
//-----------------------------------------------------------------------------

float UI::easeInOutQuad ( float t )
{
	const auto	sqr = t * t;
	return sqr / ( 2.0f * ( sqr - t ) + 1.0f );
}
//-----------------------------------------------------------------------------

juce::Colour UI::getColorWithPerceivedBrightness ( juce::Colour input, const float targetL ) noexcept
{
	auto	low = 0.0f;
	auto	high = 1.0f;
	auto	bestMatch = input;

	// 8 iterations gives ~0.0039 precision
	for ( auto i = 0; i < 8; ++i )
	{
		const auto	midV = ( low + high ) * 0.5f;
		const auto	testCol = input.withBrightness ( midV );
		const auto	currentL = testCol.getPerceivedBrightness ();

		if ( std::abs ( currentL - targetL ) <= 0.005f )
			return testCol;

		if ( currentL < targetL )
			low = midV;
		else
			high = midV;

		bestMatch = testCol;
	}

	// If the color is still too dark, try adjusting saturation instead of brightness
	if ( bestMatch.getPerceivedBrightness () < targetL - 0.005f )
	{
		auto	lowS = 0.0f;
		auto	highS = bestMatch.getSaturation ();

		for ( auto i = 0; i < 8; ++i )
		{
			const auto	midS = ( lowS + highS ) * 0.5f;
			const auto	testCol = bestMatch.withSaturation ( midS );
			const auto	currentL = testCol.getPerceivedBrightness ();

			if ( std::abs ( currentL - targetL ) <= 0.005f )
				return testCol;

			if ( currentL < targetL )
				highS = midS;
			else
				lowS = midS;

			bestMatch = testCol;
		}
	}

	return bestMatch;
}
//-----------------------------------------------------------------------------

juce::Font UI::fontSized ( const float points, const int weight )
{
	auto&	laf = static_cast<GUI_LookAndFeel&> ( juce::LookAndFeel::getDefaultLookAndFeel () );

	return laf.fontPoints ( points, weight );
}
//-----------------------------------------------------------------------------

juce::Font UI::font ( const UI::fonts::Role role )
{
	const auto	def = fontDef ( role );

	return fontSized ( def.size, def.weight );
}
//-----------------------------------------------------------------------------

UI::fonts::Def UI::fontDef ( const UI::fonts::Role role )
{
	const juce::SharedResourcePointer<Theme>	theme;

	return theme->getFontDef ( role );
}
//-----------------------------------------------------------------------------

float UI::corner ( const UI::corners::Role role, const float shorterSide )
{
	const juce::SharedResourcePointer<Theme>	theme;

	const auto	radius = theme->getCornerRadius ( role );

	return shorterSide > 0.0f ? std::min ( radius, shorterSide / 2.0f ) : radius;
}
//-----------------------------------------------------------------------------

float UI::corner ( const UI::corners::Role role, const juce::Rectangle<float>& r )
{
	return corner ( role, std::min ( r.getWidth (), r.getHeight () ) );
}
//-----------------------------------------------------------------------------

float UI::lineWidth ( const UI::lines::Role role )
{
	const juce::SharedResourcePointer<Theme>	theme;

	return theme->getLineWidth ( role );
}
//-----------------------------------------------------------------------------

juce::Rectangle<float> UI::padded ( const juce::Rectangle<float>& r, const UI::paddings::Role role )
{
	const auto	p = paddingDef ( role );

	return { r.getX () + p.left,
			 r.getY () + p.top,
			 r.getWidth () - p.left - p.right,
			 r.getHeight () - p.top - p.bottom };
}
//-----------------------------------------------------------------------------

UI::paddings::Def UI::paddingDef ( const UI::paddings::Role role )
{
	const juce::SharedResourcePointer<Theme>	theme;

	return theme->getPaddingDef ( role );
}
//-----------------------------------------------------------------------------

void UI::setFontRole ( juce::Label& label, const UI::fonts::Role role )
{
	label.getProperties ().set ( "fontRole", int ( role ) );

	// Also set concretely, for measurement paths that read the font directly
	label.setFont ( font ( role ) );
}
//-----------------------------------------------------------------------------

juce::Font UI::monoFont ( const float points, const int weight )
{
	auto&	laf = static_cast<GUI_LookAndFeel&> ( juce::LookAndFeel::getDefaultLookAndFeel () );

	return laf.monoFontPoints ( points, weight );
}
//-----------------------------------------------------------------------------

std::unique_ptr<juce::Drawable> UI::getMenuIcon ( const juce::String& name )
{
	auto	[ iconImage, _ ] = UI::getSVG ( name );

	iconImage->replaceColour ( juce::Colours::black, endColor );

	return std::move ( iconImage );
}
//-----------------------------------------------------------------------------

juce::PopupMenu::Item UI::newMenuItem ( const juce::String& name, const juce::String& icon, std::function<void ()> func )
{
	juce::PopupMenu::Item	item ( name );

	item.setAction ( std::move ( func ) );
	item.setImage ( getMenuIcon ( icon ) );

	return item;
}
//-----------------------------------------------------------------------------

juce::PopupMenu::Item UI::newDangerousMenuItem ( const juce::String& name, const juce::String& icon, std::function<void ()> func )
{
	auto	item = newMenuItem ( name, icon, std::move ( func ) );

	return item.setColour ( juce::Colours::red );
}
//-----------------------------------------------------------------------------

void UI::setLayout ( gin::LayoutSupport& layout, const juce::StringArray& paths )
{
	if ( ! datasource::isPak () )
	{
		juce::Array<juce::File>	files;
		for ( const auto& p : paths )
			files.add ( datasource::getDevFile ( p ) );

		layout.setLayout ( files );
		return;
	}

	juce::StringArray	texts;
	for ( const auto& p : paths )
		texts.add ( datasource::loadText ( p ) );

	layout.setLayouts ( texts );
}
//-----------------------------------------------------------------------------

std::pair<std::unique_ptr<juce::Drawable>, int> UI::getSVG ( const juce::String& svgName )
{
	auto	svgStr = datasource::loadText ( "UI/svg/" + svgName + ".svg" );
	jassert ( svgStr.isNotEmpty () );

	auto	svg = juce::XmlDocument::parse ( svgStr );
	jassert ( svg );

	// Missing or unparsable: callers dereference the drawable unconditionally,
	// so return a valid empty one
	if ( ! svg )
		return { std::make_unique<juce::DrawableComposite> (), 1 };

	auto	viewBoxSize = 0;
	if ( auto vbStr = svg->getStringAttribute ( "viewBox" ); vbStr.isNotEmpty () )
	{
		auto	vb = juce::Rectangle<int>::fromString ( vbStr );
		viewBoxSize = std::max ( vb.getWidth (), vb.getHeight () );
	}
	else if ( auto w = svg->getIntAttribute ( "width" ), h = svg->getIntAttribute ( "height" ); w > 0 && h > 0 )
	{
		viewBoxSize = std::max ( w, h );
	}

	jassert ( viewBoxSize > 0 );

	auto	drawable = juce::Drawable::createFromSVGString ( svgStr );
	if ( ! drawable )
		return { std::make_unique<juce::DrawableComposite> (), 1 };

	return { std::move ( drawable ), std::max ( viewBoxSize, 1 ) };
}
//-----------------------------------------------------------------------------

juce::Path& UI::getScaledPath ( const juce::String& resourceName, juce::Rectangle<float> rect, juce::RectanglePlacement placement /*= 0*/, float padding /* = 0.0f */ )
{
	// The path cache below is unlocked, message thread only
	JUCE_ASSERT_MESSAGE_THREAD

	padding *= std::min ( rect.getWidth (), rect.getHeight () );
	rect.reduce ( padding, padding );

	//
	// Cache
	//
	struct cacheEntry
	{
		juce::Rectangle<float>		rect;
		juce::RectanglePlacement	placement;
		juce::Path					path;
	};

	static juce::HashMap<juce::String, cacheEntry>	paths;

	// Already in cache?
	if ( paths.contains ( resourceName ) )
	{
		auto&	p = paths.getReference ( resourceName );

		// Cache matches sizes?
		if ( p.rect == rect && p.placement == placement )
			return p.path;
	}

	auto	[ drawable, _ ] = getSVG ( resourceName );

	//
	// Convert SVG to scaled path
	//
	drawable->setDrawableTransformToFit ( rect, placement );
	auto	path = drawable->getOutlineAsPath ();

	//
	// Store in cache
	//
	paths.set ( resourceName, { rect, placement, path } );
	return paths.getReference ( resourceName ).path;
}
//-----------------------------------------------------------------------------

juce::Path& UI::getScaledPathWithSize ( const juce::String& resourceName, juce::Rectangle<float> rect, juce::RectanglePlacement placement /*= 0*/, float padding /* = 0.0f */ )
{
	// The path cache below is unlocked, message thread only
	JUCE_ASSERT_MESSAGE_THREAD

	padding *= std::min ( rect.getWidth (), rect.getHeight () );
	rect.reduce ( padding, padding );

	//
	// Cache
	//
	struct cacheEntry
	{
		juce::Rectangle<float>		rect;
		juce::RectanglePlacement	placement;
		juce::Path					path;
	};

	static juce::HashMap<juce::String, cacheEntry>	paths;

	// Already in cache?
	if ( paths.contains ( resourceName ) )
	{
		auto& p = paths.getReference ( resourceName );

		// Cache matches sizes?
		if ( p.rect == rect && p.placement == placement )
			return p.path;
	}

	auto [ drawable, orgSize ] = getSVG ( resourceName );

	//
	// Convert SVG to scaled path
	//
	{
		const auto	p = drawable->getOutlineAsPath ();
		const auto	b = p.getBounds ();
		const auto	newSize = std::max ( rect.getWidth (), rect.getHeight () ) / orgSize;

		auto	newX = b.getWidth () * 0.5f * newSize;
		auto	newY = b.getHeight () * 0.5f * newSize;

		if ( placement.testFlags ( juce::RectanglePlacement::xMid ) )
			newX = rect.getCentreX ();

		if ( placement.testFlags ( juce::RectanglePlacement::yMid ) )
			newY = rect.getCentreY ();

		auto	trans = juce::AffineTransform::translation ( b.getWidth () * -0.5f - b.getX (), b.getHeight () * -0.5f - b.getY () )
						.scaled ( newSize )
						.translated ( newX, newY );

		drawable->setDrawableTransform ( trans );
	}

	auto	path = drawable->getOutlineAsPath ();

	//
	// Store in cache
	//
	paths.set ( resourceName, { rect, placement, path } );
	return paths.getReference ( resourceName ).path;
}
//-----------------------------------------------------------------------------

void UI::repaintCell ( juce::TableListBox* tlb, const int rowNumber, const int columnId )
{
	if ( tlb->getHeader ().getIndexOfColumnId ( columnId, true ) < 0 )
		return;

	auto	cp = tlb->getCellPosition ( columnId, rowNumber, true );
	if ( cp.isEmpty () )
		return;

	tlb->repaint ( cp );
}
//-----------------------------------------------------------------------------

void UI::repaintColumn ( juce::TableListBox* tlb, const int columnId )
{
	auto&	header = tlb->getHeader ();
	auto	headerCell = header.getColumnPosition ( header.getIndexOfColumnId ( columnId, true ) );

	headerCell.translate ( header.getX (), 0 );

	tlb->repaint ( headerCell.withY ( 0 ).withHeight ( tlb->getHeight () ) );
}
//-----------------------------------------------------------------------------
