#include "GUI_LookAndFeel.h"

#include "UI/ui-colors.h"

#include "std_lime/lime_math.h"

#include "ultra-shared/Config/DataSource.h"
#include "ultra-shared/Resources/Icons.h"
#include "ultra-shared/UI/Components/GUI_DesktopDropshadow.h"
#include "ultra-shared/UI/Components/GUI_Toggle.h"
#include "ultra-shared/UI/UI_Helpers.h"
#include "ultra-shared/Video/colodore.h"
#include "ultra-shared/Video/VIC2_Render.h"

//-------------------------------------------------------------------------------------------------

const juce::FontOptions& GUI_LookAndFeel::fontWeighted ( const int weight )
{
	// The cache below is unlocked, message thread only
	JUCE_ASSERT_MESSAGE_THREAD

	if ( const auto it = fontCache.find ( weight ); it != fontCache.end () )
		return it->second;

	const juce::FontVariableSetting	settings[] = { { "wght", float ( weight ) } };

	const auto	typeface = figtreeBase != nullptr ? figtreeBase->cloneWithVariableSettings ( settings ) : juce::Typeface::Ptr {};

	return fontCache.emplace ( weight, typeface != nullptr ? juce::FontOptions ( typeface ) : juce::FontOptions () ).first->second;
}
//-----------------------------------------------------------------------------

const juce::FontOptions& GUI_LookAndFeel::monoWeighted ( const int weight )
{
	// The cache below is unlocked, message thread only
	JUCE_ASSERT_MESSAGE_THREAD

	if ( const auto it = monoFontCache.find ( weight ); it != monoFontCache.end () )
		return it->second;

	const juce::FontVariableSetting	settings[] = { { "wght", float ( weight ) } };

	const auto	typeface = monoBase != nullptr ? monoBase->cloneWithVariableSettings ( settings ) : juce::Typeface::Ptr {};

	return monoFontCache.emplace ( weight, typeface != nullptr ? juce::FontOptions ( typeface ) : juce::FontOptions () ).first->second;
}
//-----------------------------------------------------------------------------

GUI_LookAndFeel::GUI_LookAndFeel ()
{
	//
	// Font
	//
	juce::Font::setDefaultMinimumHorizontalScaleFactor ( 1.0f );

	//
	// Populate fonts table
	//
	{
		// A failed font load falls back to the system font: ugly, but alive
		auto loadFont = [] ( const juce::String& name )
		{
			const auto	mb = datasource::loadData ( "UI/fonts/" + name );

			if ( mb.getSize () == 0 )
			{
				Z_ERR ( "Font file failed to load (" << name << ")" );
				return juce::Typeface::Ptr {};
			}

			auto	typeface = juce::Typeface::createSystemTypefaceFor ( mb.getData (), mb.getSize () );

			if ( typeface == nullptr )
				Z_ERR ( "Font file not parseable (" << name << ")" );

			return typeface;
		};

		// The base variable fonts; Figtree gets per-weight clones cached on
		// demand in fontWeighted (), the mono face clones in monoFontPoints ()
		figtreeBase = loadFont ( "Figtree-VariableFont_wght.ttf" );
		monoBase = loadFont ( "NotoSansMono-VariableFont_wght-subset.ttf" );	// Monospace, Latin-1 subset
	}

	defaultFont = fontWeighted ( 500 ).withHeight ( 14.0f );

	//
	// Default colors
	//
	const std::pair<const int, juce::Colour> juceDefaultColors[] =
	{
		// JUCE stuff
		{ juce::ListBox::backgroundColourId,				juce::Colours::transparentBlack },
		{ juce::ListBox::outlineColourId,					juce::Colours::transparentBlack },

		{ juce::ComboBox::backgroundColourId,				juce::Colour::fromRGB ( 33, 42, 48 ) },
		{ juce::ComboBox::buttonColourId,					juce::Colour::fromRGB ( 33, 42, 48 ) },
		{ juce::ComboBox::outlineColourId,					juce::Colours::black },
		{ juce::ComboBox::textColourId,						juce::Colours::whitesmoke },
		{ juce::ComboBox::arrowColourId,					juce::Colours::whitesmoke },

		{ juce::ToggleButton::textColourId,					juce::Colours::whitesmoke	},
		{ juce::ToggleButton::tickColourId,					juce::Colours::green		},

		{ juce::TextEditor::backgroundColourId,				juce::Colours::white },
		{ juce::TextEditor::textColourId,                 	juce::Colours::black },
		{ juce::TextEditor::highlightColourId,            	juce::Colours::cornflowerblue },
		{ juce::TextEditor::highlightedTextColourId,      	juce::Colours::white },
		{ juce::TextEditor::outlineColourId,              	juce::Colours::transparentBlack },
		{ juce::TextEditor::focusedOutlineColourId,       	juce::Colours::cornflowerblue },

		{ juce::Label::textColourId,						juce::Colours::whitesmoke },
		{ juce::Label::backgroundWhenEditingColourId,		juce::Colours::white },
		{ juce::Label::textWhenEditingColourId,				juce::Colours::black },
		{ juce::Label::outlineWhenEditingColourId,			juce::Colours::deeppink },

		{ juce::ScrollBar::backgroundColourId,				juce::Colours::black.withAlpha ( 0.25f ) },
		{ juce::ScrollBar::thumbColourId,					juce::Colours::white.withAlpha ( 0.25f ) },
		{ juce::ScrollBar::trackColourId,					juce::Colours::white.withAlpha ( 0.5f ) },

		{ juce::BubbleComponent::backgroundColourId,		juce::Colour::fromRGB ( 33, 42, 48 ) },
		{ juce::BubbleComponent::outlineColourId,			juce::Colour::fromRGB ( 0, 0, 0 ) },

		{ juce::TooltipWindow::backgroundColourId,			juce::Colour::fromRGB ( 33, 42, 48 ) },
		{ juce::TooltipWindow::textColourId,				juce::Colour ( 0xff'F5FBFF ).withAlpha ( 0.9f ) },
		{ juce::TooltipWindow::outlineColourId,				juce::Colours::black.withAlpha ( 0.1f ) },

		{ juce::PopupMenu::backgroundColourId,				juce::Colour::fromRGB ( 33, 42, 48 ) },
		{ juce::PopupMenu::textColourId,					juce::Colour ( 0xff'F5FBFF ).withAlpha ( 0.9f ) },
		{ juce::PopupMenu::highlightedBackgroundColourId,	juce::Colour::fromRGB ( 33, 42, 48 ).interpolatedWith ( juce::Colour::fromRGB ( 240, 248, 255 ), 0.2f ) },
		{ juce::PopupMenu::highlightedTextColourId,			juce::Colour ( 0xff'F5FBFF ).withAlpha ( 0.9f ) },
	};

	for ( auto [ colorId, color ] : juceDefaultColors )
		setColour ( colorId, color );
}
//----------------------------------------------------------------------------------

GUI_LookAndFeel::~GUI_LookAndFeel ()
{
}
//----------------------------------------------------------------------------------

juce::Font GUI_LookAndFeel::getPopupMenuFont ()
{
	return UI::font ( UI::fonts::drop_down );
}
//----------------------------------------------------------------------------------

juce::Font GUI_LookAndFeel::getComboBoxFont ( juce::ComboBox& )
{
	return UI::font ( UI::fonts::drop_down );
}
//----------------------------------------------------------------------------------

// The main window paints its own chrome, secondary windows keep the stock
// decorations
static bool isMainAppWindow ( const juce::Component& window )
{
	return window.getName () == juce::JUCEApplication::getInstance ()->getApplicationName ();
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::drawResizableWindowBorder ( juce::Graphics& g, int w, int h, const juce::BorderSize<int>& border, juce::ResizableWindow& window )
{
	if ( isMainAppWindow ( window ) )
		return;

	juce::LookAndFeel_V4::drawResizableWindowBorder ( g, w, h, border, window );
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::drawResizableFrame ( juce::Graphics& /*g*/, int /*w*/, int /*h*/, const juce::BorderSize<int>& /*border*/ )
{
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::drawDocumentWindowTitleBar ( juce::DocumentWindow& window, juce::Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const juce::Image* icon, bool drawTitleTextOnLeft )
{
	if ( isMainAppWindow ( window ) )
		return;

	juce::LookAndFeel_V4::drawDocumentWindowTitleBar ( window, g, w, h, titleSpaceX, titleSpaceW, icon, drawTitleTextOnLeft );
}
//----------------------------------------------------------------------------------

// The three window buttons highlight as one group: when the hovered button
// reaches the given state, the whole row follows
static void syncWindowButtons ( const juce::MouseEvent& e, const juce::Button::ButtonState state )
{
	auto	win = dynamic_cast<juce::DocumentWindow*> ( e.eventComponent->getTopLevelComponent () );
	if ( ! win )
		return;

	auto	evtBut = dynamic_cast<juce::Button*> ( e.eventComponent );
	if ( ! evtBut || evtBut->getState () != state )
		return;

	win->getCloseButton ()->setState ( state );
	win->getMinimiseButton ()->setState ( state );
	win->getMaximiseButton ()->setState ( state );
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::mouseEnter ( const juce::MouseEvent& e )
{
	syncWindowButtons ( e, juce::Button::buttonOver );
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::mouseExit ( const juce::MouseEvent& e )
{
	syncWindowButtons ( e, juce::Button::buttonNormal );
}
//----------------------------------------------------------------------------------

class OS_DocumentWindowButton : public juce::Button
{
public:
	OS_DocumentWindowButton ( const juce::String& name, juce::Colour c, const juce::Path& normal, const juce::Path& toggled, const float _reduction = 3.0f )
		: juce::Button ( name )
		, colour ( c )
		, normalShape ( normal )
		, toggledShape ( toggled )
		, reduction ( _reduction )
	{
	}

	void paintButton ( juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown ) override
	{
		#if JUCE_MAC
			const auto	hasFocus = getCurrentlyFocusedComponent () != nullptr;
			auto	b = getLocalBounds ().toFloat ().reduced ( 0.8f );

			b = b.withSizeKeepingCentre ( b.getHeight (), b.getHeight () );

			auto drawCircle = [ &g, &b ] ( const juce::Colour c1, const juce::Colour c2 )
			{
				g.setColour ( c1 );
				g.fillEllipse ( b );
				g.setColour ( c2 );
				g.drawEllipse ( b.reduced ( 0.125f ), 0.25f );
			};

			if ( hasFocus )
			{
				auto	c = colour.interpolatedWith ( juce::Colours::black, shouldDrawButtonAsDown ? 0.2f : 0.0f );
				drawCircle ( c, c.withMultipliedSaturation ( 2.0f ) );

				if ( shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown )
				{
					g.setColour ( juce::Colours::black.withAlpha ( 0.5f ) );
					g.fillPath ( toggledShape, toggledShape.getTransformToScaleToFit ( b.reduced ( reduction ), true ) );
				}
			}
			else
				drawCircle ( juce::Colours::white.withAlpha ( 0.2f ), juce::Colours::black.withAlpha ( 0.06f ) );
		#else
			const auto	alpha = ( ! isEnabled () || shouldDrawButtonAsDown ) ? 0.6f : 1.0f;

			// Show background
			if ( shouldDrawButtonAsHighlighted )
				g.fillAll ( colour.withMultipliedAlpha ( alpha ) );

			// Show path
			g.setColour ( juce::Colours::white.withAlpha ( alpha ) );

			auto&	p = getToggleState () ? toggledShape : normalShape;

 			const auto	reducedRect =	juce::Justification ( juce::Justification::centred )
 										.appliedToRectangle ( juce::Rectangle<int> ( getHeight (), getHeight () ), getLocalBounds () )
 										.toFloat ()
 										.reduced ( getHeight () * 0.33f );

			g.fillPath ( p, p.getTransformToScaleToFit ( reducedRect, true ) );
		#endif
	}

private:
	juce::Colour	colour;
	juce::Path		normalShape;
	juce::Path		toggledShape;
	[[ maybe_unused ]] const float		reduction;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( OS_DocumentWindowButton )
};
//-----------------------------------------------------------------------------

juce::Button* GUI_LookAndFeel::createDocumentWindowButton ( int buttonType )
{
	juce::Path shape;

	#if JUCE_MAC
		juce::Path	emptyShape;

		constexpr auto	lineThickness = 3.5f;
		const auto		stroke = juce::PathStrokeType ( lineThickness, juce::PathStrokeType::JointStyle::mitered, juce::PathStrokeType::EndCapStyle::rounded );

		if ( buttonType == juce::DocumentWindow::closeButton )
		{
			shape.startNewSubPath ( { 0.0f, 0.0f } );
			shape.lineTo ( { 10.0f, 10.0f } );
			shape.closeSubPath ();

			shape.startNewSubPath ( { 10.0f, 0.0f } );
			shape.lineTo ( { 0.0f, 10.0f } );
			shape.closeSubPath ();

			stroke.createStrokedPath ( shape, shape );

			auto	but = new OS_DocumentWindowButton ( "close", juce::Colour ( 0xff'ff6159 ), emptyShape, shape, 3.125f );

			but->addMouseListener ( this, false );

			return but;
		}

		if ( buttonType == juce::DocumentWindow::minimiseButton )
		{
			shape.startNewSubPath ( { 0.0f, 5.0f } );
			shape.lineTo ( { 10.0f, 5.0f } );
			shape.closeSubPath ();

			stroke.createStrokedPath ( shape, shape );

			auto	but = new OS_DocumentWindowButton ( "minimise", juce::Colour ( 0xff'ffbd2e ), emptyShape, shape );

			but->addMouseListener ( this, false );

			return but;
		}

		if ( buttonType == juce::DocumentWindow::maximiseButton )
		{
			constexpr auto	size = 10.0f;
			constexpr auto	offset = 1.5f;
			constexpr auto	curve = 2.5f;

			shape.startNewSubPath ( { size - offset, 0.0f } );
			shape.lineTo ( { curve, 0.0f } );
			shape.quadraticTo ( { 0.0f, 0.0f }, { 0.0f, curve } );
			shape.lineTo ( { 0.0f, size - offset } );
			shape.closeSubPath ();

			shape.startNewSubPath ( { size, offset } );
			shape.lineTo ( { size, size - curve } );
			shape.quadraticTo ( { size, size }, { size - curve, size } );
			shape.lineTo ( { offset, size } );
			shape.closeSubPath ();

			auto	but = new OS_DocumentWindowButton ( "maximise", juce::Colour ( 0xff'28c941 ), emptyShape, shape, 3.5f );

			but->addMouseListener ( this, false );

			return but;
		}
	#else
		constexpr auto	lineThickness = 0.12f;

		if ( buttonType == juce::DocumentWindow::closeButton )
		{
			shape.addLineSegment ( { 0.0f, 0.0f, 1.0f, 1.0f }, lineThickness );
			shape.addLineSegment ( { 1.0f, 0.0f, 0.0f, 1.0f }, lineThickness );

			return new OS_DocumentWindowButton ( "close", juce::Colours::red, shape, shape );
		}

		if ( buttonType == juce::DocumentWindow::minimiseButton )
		{
			shape.addLineSegment ( { 0.0f, 0.5f, 1.0f, 0.5f }, lineThickness );

			return new OS_DocumentWindowButton ( "minimise", juce::Colours::white.withAlpha ( 0.2f ), shape, shape );
		}

		if ( buttonType == juce::DocumentWindow::maximiseButton )
		{
			shape.addRectangle ( 0.0f, 0.0f, 1.0f, 1.0f );
			juce::PathStrokeType ( lineThickness ).createStrokedPath ( shape, shape );

			juce::Path fullscreenShape;
			fullscreenShape.addRectangle ( 0.0f, 0.0f, 1.0f, 1.0f );

			constexpr auto	offset = 0.3f;

			fullscreenShape.startNewSubPath ( offset, 0.0f );
			fullscreenShape.lineTo ( offset, -offset );
			fullscreenShape.lineTo ( 1.0f + offset, -offset );
			fullscreenShape.lineTo ( 1.0f + offset, 1.0f - offset );
			fullscreenShape.lineTo ( 1.0f, 1.0f - offset );

			juce::PathStrokeType ( lineThickness * ( 1.0f + offset ) ).createStrokedPath ( fullscreenShape, fullscreenShape );

			return new OS_DocumentWindowButton ( "maximise", juce::Colours::white.withAlpha ( 0.2f ), shape, fullscreenShape );
		}
	#endif

	jassertfalse;
	return nullptr;
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::positionDocumentWindowButtons ( juce::DocumentWindow& window,
													  int titleBarX, int titleBarY,
													  int titleBarW, int /*titleBarH*/,
													  juce::Button* minimiseButton,
													  juce::Button* maximiseButton,
													  juce::Button* closeButton,
													  bool positionTitleBarButtonsOnLeft )
{
	if ( positionTitleBarButtonsOnLeft )
	{
		titleBarX += 15;
		titleBarY += 12;
	}

	// Adjust positions and sizes
	const auto	border = window.getBorderThickness ();

	titleBarX -= border.getLeft ();
	titleBarY -= border.getTop ();
	titleBarW += border.getLeftAndRight ();

	const auto	buttonW = positionTitleBarButtonsOnLeft ? 15 : 44;
	const auto	buttonH = positionTitleBarButtonsOnLeft ? 15 : 30;

	auto	x = positionTitleBarButtonsOnLeft ? titleBarX : titleBarX + titleBarW - buttonW;

	if ( closeButton )
	{
		closeButton->setBounds ( x, titleBarY, buttonW, buttonH );
		x += positionTitleBarButtonsOnLeft ? buttonW : -buttonW;
	}

	if ( positionTitleBarButtonsOnLeft )
		std::swap ( minimiseButton, maximiseButton );

	if ( maximiseButton )
	{
		if ( positionTitleBarButtonsOnLeft )
		{
			maximiseButton->setBounds ( x, titleBarY, buttonW + 16, buttonH );
			x += buttonW + 16;
		}
		else
		{
			maximiseButton->setBounds ( x, titleBarY, buttonW, buttonH );
			x += -buttonW;
		}
	}

	if ( minimiseButton )
		minimiseButton->setBounds ( x, titleBarY, buttonW, buttonH );
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::positionComboBoxText ( juce::ComboBox& box, juce::Label& label )
{
	auto	f = getComboBoxFont ( box );

	auto	b = box.getLocalBounds ().reduced ( int ( f.getAscent () * 0.8f ), 0 );

	const bool	drawArrow = box.getProperties ().getWithDefault ( "drawArrow", true );
	if ( drawArrow )
		label.setBounds ( b.withTrimmedRight ( b.getHeight () ) );
	else
		label.setBounds ( b );

	label.setInterceptsMouseClicks ( false, false );
	label.setFont ( f );
}
//----------------------------------------------------------------------------------

// The selected row's menu image; the closed box shows it as a leading icon
static const juce::Drawable* selectedComboIcon ( juce::ComboBox& box )
{
	const auto	id = box.getSelectedId ();
	if ( id == 0 )
		return nullptr;

	for ( juce::PopupMenu::MenuItemIterator it ( *box.getRootMenu (), true ); it.next (); )
		if ( const auto& item = it.getItem (); item.itemID == id )
			return item.image.get ();

	return nullptr;
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawLabel ( juce::Graphics& g, juce::Label& l )
{
	auto	textArea = l.getLocalBounds ();

	// Make sure the correct color is used
	if ( auto box = dynamic_cast<juce::ComboBox*> ( l.getParentComponent () ) )
	{
		g.setColour ( box->findColour ( juce::ComboBox::textColourId, true ) );
		g.setFont ( getComboBoxFont ( *box ) );

		if ( selectedComboIcon ( *box ) != nullptr )
			textArea.removeFromLeft ( box->getHeight () * 0.94f - l.getX () );
	}
	else
	{
		g.setColour ( l.findColour ( juce::Label::textColourId, true ) );
		g.setFont ( getLabelFont ( l ) );
	}

	auto text = l.getText ();

	g.drawFittedText ( text, textArea, l.getJustificationType (),
					   std::max ( 1, int ( textArea.getHeight () / g.getCurrentFont ().getHeight () ) ),
					   l.getMinimumHorizontalScale () );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawComboBox ( juce::Graphics& g, int width, int height, bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box )
{
	auto	boxBounds = juce::Rectangle<float> { float ( width ), float ( height ) };

	auto	backCol = box.findColour ( juce::ComboBox::backgroundColourId, true );
	auto	arrowCol = box.findColour ( juce::ComboBox::arrowColourId, true );

	g.setColour ( backCol );
	g.fillRoundedRectangle ( boxBounds, UI::corner ( UI::corners::drop_down, boxBounds ) );

	// Leading icon
	if ( auto icon = selectedComboIcon ( box ) )
		icon->drawWithin ( g, boxBounds.withWidth ( boxBounds.getHeight () ).reduced ( boxBounds.getHeight () * 0.3f ), juce::RectanglePlacement::centred, 1.0f );

	// Draw arrow
	const bool	drawArrow = box.getProperties ().getWithDefault ( "drawArrow", true );
	if ( drawArrow )
	{
		const juce::SharedResourcePointer<Icons>	icons;

		auto&	p = UI::getScaledPath ( icons->get ( "laf/combo_arrow" ), boxBounds.removeFromRight ( boxBounds.getHeight () ), 0, 0.3f );

		g.setColour ( arrowCol );
		g.fillPath ( p );
	}
}
//----------------------------------------------------------------------------------

void GUI_LookAndFeel::drawButtonBackground ( juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/, bool isHover, bool /*isDown*/ )
{
	const auto	bounds = button.getLocalBounds ().toFloat ();

	g.setColour ( findColour ( isHover ? UI::colors::text : UI::colors::textMuted ) );
	drawOutline ( g, bounds, bounds.getHeight () / 2.0f, UI::lineWidth ( UI::lines::settings_location_button ) );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawButtonText ( juce::Graphics& g, juce::TextButton& button, bool /*isHover*/, bool /*isDown*/ )
{
	auto	bounds = button.getLocalBounds ().toFloat ();

	g.setColour ( findColour ( UI::colors::text ) );
	g.setFont ( UI::font ( UI::fonts::settings_location_button ) );
	g.drawText ( button.getButtonText (), bounds, juce::Justification::centred, false );
}
//-------------------------------------------------------------------------------------------------

int GUI_LookAndFeel::getDefaultScrollbarWidth ()
{
	return 16 + 6;
}
//-------------------------------------------------------------------------------------------------

int GUI_LookAndFeel::getMinimumScrollbarThumbSize ( juce::ScrollBar& sb )
{
	return juce::LookAndFeel_V4::getMinimumScrollbarThumbSize ( sb ) / 2;
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawScrollbar ( juce::Graphics& g, juce::ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown )
{
	const auto	withThumb = isScrollbarVertical ? ( height != thumbSize ) : ( width != thumbSize );
	const auto	showFull = ( isMouseOver || isMouseDown ) && withThumb;

	const auto	reducedBackX = showFull ? 0.0f : 3.0f;
	const auto	reducedThumbX = showFull ? 3.0f : 5.5f;
	constexpr auto	reducedThumbY = 3.0f;

	constexpr auto	paddingTopOrLeft = 6;
	if ( isScrollbarVertical )
	{
		x += paddingTopOrLeft;
		width -= paddingTopOrLeft;
	}
	else
	{
		y += paddingTopOrLeft;
		height -= paddingTopOrLeft;
	}

	// Background
	if ( const auto col = scrollbar.findColour ( juce::ScrollBar::backgroundColourId, true ); ! col.isTransparent () )
	{
		g.setColour ( col );

		const auto	r = juce::Rectangle<int> ( x, y, width, height ).toFloat ().reduced ( reducedBackX, 0.0f );
		const auto	borderRadius = ( isScrollbarVertical ? r.getWidth () : r.getHeight () ) / 2.0f;

		g.fillRoundedRectangle ( r, borderRadius );
	}

	// Thumb
	if ( withThumb )
	{
		const auto	thumbBounds = isScrollbarVertical ?
												juce::Rectangle<int> ( x, thumbStartPosition, width, thumbSize )
											:	juce::Rectangle<int> ( thumbStartPosition, y, thumbSize, height );

		auto	trackCol = scrollbar.findColour ( juce::ScrollBar::trackColourId, true );
		if ( ! isMouseDown )
			trackCol = scrollbar.findColour ( juce::ScrollBar::thumbColourId, true ).interpolatedWith ( trackCol, isMouseOver ? 0.5f : 0.0f );

		g.setColour ( trackCol );

		const auto	fThumbBounds = thumbBounds.toFloat ().reduced ( reducedThumbX, reducedThumbY );
		const auto	borderRadius = ( isScrollbarVertical ? fThumbBounds.getWidth () : fThumbBounds.getHeight () ) / 2.0f;

		g.fillRoundedRectangle ( fThumbBounds, borderRadius );
	}
}
//----------------------------------------------------------------------------------

juce::TextLayout GUI_LookAndFeel::layoutTooltipText ( const juce::String& text ) noexcept
{
	constexpr auto	maxToolTipWidth = 1000.0f;

	juce::AttributedString	s;
	const auto	font = UI::font ( UI::fonts::tooltip );

	const auto	bck = findColour ( juce::TooltipWindow::backgroundColourId );
	const auto	col = findColour ( juce::TooltipWindow::textColourId );

	s.setJustification ( juce::Justification::centredLeft );

	// Add each line separately, so we have more control over colors etc.
	{
		auto	arr = juce::StringArray::fromLines ( text );
		for ( auto& str : arr )
		{
			const auto	other = str.startsWithChar ( '#' ) ? 0.5f : 0.0f;

			s.append ( str.replaceFirstOccurrenceOf ( "#", "" ) + "\n", font, col.interpolatedWith ( bck, other ) );
		}
	}

	juce::TextLayout	tl;
	tl.createLayoutWithBalancedLineLengths ( s, maxToolTipWidth );
	return tl;
}
//-------------------------------------------------------------------------------------------------

juce::Rectangle<int> GUI_LookAndFeel::getTooltipBounds ( const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea )
{
	const juce::TextLayout	tl ( layoutTooltipText ( tipText ) );

	// The mouse offsets and the drop-shadow blur margin scale with the themed
	// tooltip font; the text padding inside the box is themed itself
	const auto	tooltipFontSize = UI::font ( UI::fonts::tooltip ).getHeight ();
	const auto	pad = UI::paddingDef ( UI::paddings::tooltip );

	const auto	w = int ( tl.getWidth () + pad.left + pad.right + tooltipFontSize );
	const auto	h = int ( tl.getHeight () + pad.top + pad.bottom + tooltipFontSize );

	return juce::Rectangle<int> ( screenPos.x > parentArea.getCentreX () ?
											  screenPos.x - int ( w )
											  : int ( float ( screenPos.x ) + tooltipFontSize ),
								  screenPos.y > parentArea.getCentreY () ?
											  screenPos.y - int ( float ( h ) + tooltipFontSize / 3)
											  : screenPos.y + int ( tooltipFontSize / 3 ),
								  w, h )
								  .constrainedWithin ( parentArea );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawTooltip ( juce::Graphics& g, const juce::String& text, int width, int height )
{
	const auto	b = juce::Rectangle<float>	( float ( width ), float ( height ) );
	const auto	tooltipFontSize = UI::font ( UI::fonts::tooltip ).getHeight ();
	const auto	radius = UI::corner ( UI::corners::tooltip, b.reduced ( tooltipFontSize / 2.0f ) );
	const auto	offset = juce::Point<float> ( 0.0f, -( tooltipFontSize / 8.0f ) );

	// Perfect drop-shadow
	{
		auto	img = juce::Image ( juce::Image::PixelFormat::SingleChannel, width, height, true );

		{
			juce::Graphics	ig ( img );

			ig.setColour ( juce::Colours::white.withAlpha ( 0.5f ) );
			ig.fillRoundedRectangle ( b.reduced ( tooltipFontSize / 2.0f ), radius );
		}

		gin::applyStackBlur ( img, int ( tooltipFontSize / 2.0f ) );

		g.setColour ( juce::Colours::black );
		g.drawImage ( img, b, 0, true );
	}

	g.setColour ( findColour ( juce::TooltipWindow::backgroundColourId ) );
	g.fillRoundedRectangle ( b.reduced ( tooltipFontSize / 2.0f + 1.0f ) + offset, radius );

	if ( auto outCol = findColour ( juce::TooltipWindow::outlineColourId ); ! outCol.isTransparent () )
	{
		if ( const auto outlineW = UI::lineWidth ( UI::lines::tooltip ); UI::lines::visible ( outlineW ) )
		{
			g.setColour ( outCol );
			g.drawRoundedRectangle ( b.reduced ( tooltipFontSize / 2.0f + outlineW / 2.0f ) + offset, radius, outlineW );
		}
	}

	const juce::TextLayout	tl ( layoutTooltipText ( text ) );

	// The text sits at the themed padding inside the box (the window bounds
	// minus the blur margin, see getTooltipBounds)
	const auto	textArea = UI::padded ( b.reduced ( tooltipFontSize / 2.0f ), UI::paddings::tooltip );

	tl.draw ( g, textArea + offset );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::fillTextEditorBackground ( juce::Graphics& g, int width, int height, juce::TextEditor& textEditor )
{
	const auto	col = textEditor.findColour ( juce::TextEditor::backgroundColourId );

	g.setColour ( col );

	const auto	r = juce::Rectangle<float> { float ( width ), float ( height ) }.reduced ( 1.0f );

	g.fillRoundedRectangle ( r, UI::corner ( UI::corners::text_editor, r ) );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawTextEditorOutline ( juce::Graphics& g, int width, int height, juce::TextEditor& textEditor )
{
	if ( ! textEditor.hasKeyboardFocus ( true ) )
		return;

	const auto	col = textEditor.findColour ( juce::TextEditor::focusedOutlineColourId );
	if ( col.isTransparent () )
		return;

	g.setColour ( col );

	const auto	b = juce::Rectangle<float> { float ( width ) , float ( height ) };

	drawOutline ( g, b, UI::corner ( UI::corners::text_editor, b ), UI::lineWidth ( UI::lines::text_editor ) );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawTableHeaderBackground ( juce::Graphics& /*g*/, juce::TableHeaderComponent& /*header*/ )
{
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawTableHeaderColumn ( juce::Graphics& g, juce::TableHeaderComponent& header, const juce::String& columnName, int columnId, int width, int height, bool isMouseOver, bool /*isMouseDown*/, int columnFlags )
{
//	height -= 10;

	auto&	props = header.getProperties ();
	const float	xOff = props.getWithDefault ( "colOff" + juce::String ( columnId ), 4.0f );

	const auto	colId = isMouseOver && ( columnFlags & juce::TableHeaderComponent::sortable ) ? UI::colors::text : UI::colors::textMuted;
	g.setColour ( findColour ( colId ) );

	const auto	fnt = UI::font ( UI::fonts::table_header );
	const auto	th = float ( height );
	float		tw = float ( width ) - xOff;

	const juce::String	icon = props.getWithDefault ( "colIcon" + juce::String ( columnId ), juce::String () );

	const auto	colName = columnName.toUpperCase ();

	if ( ( columnFlags & ( juce::TableHeaderComponent::sortedForwards | juce::TableHeaderComponent::sortedBackwards ) ) != 0 )
	{
		tw = width - th;

		if ( icon.isEmpty () )
			tw = std::min ( juce::GlyphArrangement::getStringWidth ( fnt, colName ), tw );

		auto	area = juce::Rectangle<float> { tw, 0.0f, th, th };

		juce::Path sortArrow;

		sortArrow.startNewSubPath ( 0.0f, 0.0f );
		sortArrow.lineTo ( 0.5f, ( columnFlags & juce::TableHeaderComponent::sortedForwards ) ? -0.5f : 0.5f );
		sortArrow.lineTo ( 1.0f, 0.0f );

		g.strokePath ( sortArrow, juce::PathStrokeType ( 1.0f ), sortArrow.getTransformToScaleToFit ( area.removeFromRight ( th / 2.0f ).reduced ( 3.0f ), true ) );
	}

	if ( icon.isEmpty () )
	{
		const int	just = props.getWithDefault ( "colJust" + juce::String ( columnId ), int ( juce::Justification::Flags::centredLeft ) );
		g.setFont ( fnt );
		g.drawText ( colName, juce::Rectangle<float> { xOff, 0.0f, tw, th }, just, true);
	}
	else
	{
		const float	yOff = props.getWithDefault ( "colOffY" + juce::String ( columnId ), 3.0f );

		g.fillPath ( UI::getScaledPath ( icon, juce::Rectangle<float> { xOff, yOff, 14.0f, 14.0f }, juce::RectanglePlacement::centred ) );
	}
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::preparePopupMenuWindow ( juce::Component& newWindow )
{
	newWindow.setOpaque ( false );
	new GUI_DesktopDropshadow ( newWindow );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawPopupMenuBackground ( juce::Graphics& g, int width, int height )
{
	const auto	rc = juce::Rectangle<float> { float ( width ), float ( height ) };
	const auto	radius = UI::corner ( UI::corners::menu_body, rc );
	const auto	colour = findColour ( juce::PopupMenu::backgroundColourId );

	g.setColour ( colour );
	drawOutlinedRect ( g, rc, radius, UI::lineWidth ( UI::lines::menu_body ), colour.darker () );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawPopupMenuItem ( juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* obs_textColour )
{
	const auto	destructive = obs_textColour;

	const auto	bckCol = findColour ( juce::PopupMenu::backgroundColourId );
	const auto	txtCol = findColour ( juce::PopupMenu::textColourId );
	const auto	bckHighCol = findColour ( destructive ? UI::colors::statusError : int ( juce::PopupMenu::highlightedBackgroundColourId ) );
	const auto	txtHighCol = findColour ( destructive ? UI::colors::text : int ( juce::PopupMenu::highlightedTextColourId ) );

	if ( isSeparator )
	{
		//
		// Separator
		//
		auto	r = area.toFloat ().withSizeKeepingCentre ( area.getWidth () - 20.0f, UI::lineWidth ( UI::lines::menu_separator ) );

		g.setColour ( txtCol.interpolatedWith ( bckCol, 0.9f ) );
		g.fillRect ( r );
	}
	else
	{
		auto r = area.toFloat ().reduced ( 2.5f, 0.0f );
		auto backRect = r;

		auto font = getPopupMenuFont ();
		auto maxFontHeight = r.getHeight () / 1.3f;

		if ( font.getHeight () > maxFontHeight )
			font.setHeight ( maxFontHeight );

		auto	forCol = txtCol;
		auto	bakCol = bckCol;

		if ( isHighlighted && isActive )
		{
			forCol = txtHighCol;
			bakCol = bckHighCol;

			g.setColour ( bakCol );

			const auto	highlightRect = backRect.reduced ( maxFontHeight / 4.0f, 0.5f );
			g.fillRoundedRectangle ( highlightRect, UI::corner ( UI::corners::menu_highlight, highlightRect ) );
		}

		g.setColour ( forCol.interpolatedWith ( bakCol, isActive ? 0.0f : 0.75f ) );

		r.reduce ( std::min ( 5.0f, float ( area.getWidth () ) / 20.0f ), 0.0f );

		g.setFont ( font );

		auto iconArea = r.removeFromLeft ( maxFontHeight );

		if ( icon )
		{
			const auto	iconSize = font.getAscent () * 0.8f;

			const auto	iconOpacity = ( isHighlighted && destructive ) ? 2.5f : 1.0f;

			icon->drawWithin ( g, iconArea.withSizeKeepingCentre ( iconSize, iconSize ), juce::RectanglePlacement::centred, ( isActive ? 0.4f : 0.2f ) * iconOpacity );
		}
		else if ( isTicked )
		{
			if ( hasSubMenu )
			{
				g.fillEllipse ( iconArea.withSizeKeepingCentre ( iconArea.getWidth () * 0.45f, iconArea.getWidth () * 0.45f ) );
			}
			else
			{
				const juce::SharedResourcePointer<Icons>	icons;

				auto&	p = UI::getScaledPath ( icons->get ( "laf/check" ), iconArea, 0, 0.32f );
				g.fillPath ( p );
			}
		}

		if ( hasSubMenu )
		{
			const juce::SharedResourcePointer<Icons>	icons;

			auto&	p = UI::getScaledPath ( icons->get ( "laf/submenu_arrow" ), r.removeFromRight ( r.getHeight () ), 0, 0.32f );
			g.fillPath ( p );
		}

		r.removeFromRight ( 3.0f );
		g.drawText ( text, r, juce::Justification::centredLeft, false );

		if ( shortcutKeyText.isNotEmpty () )
		{
			auto f2 = font;
			f2.setHeight ( f2.getHeight () * 0.75f );
			f2.setHorizontalScale ( 0.95f );
			g.setFont ( f2 );

			g.drawText ( shortcutKeyText, r, juce::Justification::centredRight, true );
		}
	}
}
//-------------------------------------------------------------------------------------------------

int GUI_LookAndFeel::getPopupMenuBorderSizeWithOptions ( const juce::PopupMenu::Options& )
{
	return 7;
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::getIdealPopupMenuItemSize ( const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight )
{
	if ( isSeparator )
	{
		idealWidth = 50;
		idealHeight = 15;
		return;
	}

	juce::LookAndFeel_V4::getIdealPopupMenuItemSize ( text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight );

	idealWidth += juce::roundToInt ( getPopupMenuFont ().getHeight () * 2.2f );
	idealHeight = juce::roundToInt ( getPopupMenuFont ().getHeight () * 2.5f );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawPopupMenuSectionHeader ( juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName )
{
	const auto	txtCol = findColour ( juce::PopupMenu::backgroundColourId );
	const auto	bckCol = findColour ( juce::PopupMenu::textColourId ).interpolatedWith ( txtCol, 0.5f );

	const auto	r = area.toFloat ().reduced ( 10.0f, 6.0f );

	g.setColour ( bckCol );
	g.fillRoundedRectangle ( r, 1.5f );

	g.setFont ( fontWithHeight ( r.getHeight () / 1.6f ) );
	g.setColour ( txtCol );
	g.drawText ( sectionName, r.reduced ( 10.0f, 0.0f), juce::Justification::centredLeft, true );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawPopupMenuColumnSeparatorWithOptions ( juce::Graphics& g, const juce::Rectangle<int>& area, const juce::PopupMenu::Options& /*opt*/ )
{
	const auto	bckCol = findColour ( juce::PopupMenu::backgroundColourId );
	const auto	txtCol = findColour ( juce::PopupMenu::textColourId );

	const	auto	margin = ( float )area.getWidth () / 2.0f;

	auto r = area.toFloat ().reduced ( margin - UI::lineWidth ( UI::lines::menu_column_separator ) / 2.0f, margin * 2.0f );

	g.setColour ( txtCol.interpolatedWith ( bckCol, 0.9f ) );
	g.fillRect ( r );
}
//-------------------------------------------------------------------------------------------------

int GUI_LookAndFeel::getPopupMenuColumnSeparatorWidthWithOptions ( const juce::PopupMenu::Options& )
{
	return 4;
}
//-------------------------------------------------------------------------------------------------

// The bubble outline with the tip notch spliced into the top or bottom edge
static juce::Path bubblePath ( const float w, const float h, const float corner, const float tipSize, const bool tipAbove )
{
	const auto	c = w / 2.0f;

	juce::Path	p;

	p.startNewSubPath ( { corner, 0.0f } );

	if ( tipAbove )
	{
		p.lineTo ( { c - tipSize, 0.0f } );
		p.lineTo ( { c, -tipSize } );
		p.lineTo ( { c + tipSize, 0.0f } );
	}

	p.lineTo ( { w - corner, 0.0f } );
	p.quadraticTo ( { w, 0.0f }, { w, corner } );
	p.lineTo ( { w, h - corner } );
	p.quadraticTo ( { w, h }, { w - corner, h } );

	if ( ! tipAbove )
	{
		p.lineTo ( { c + tipSize, h } );
		p.lineTo ( { c, h + tipSize } );
		p.lineTo ( { c - tipSize, h } );
	}

	p.lineTo ( { corner, h } );
	p.quadraticTo ( { 0.0f, h }, { 0.0f, h - corner } );
	p.lineTo ( { 0.0f, corner } );
	p.quadraticTo ( { 0.0f, 0.0f }, { corner, 0.0f } );
	p.closeSubPath ();

	return p;
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawBubble ( juce::Graphics& g, juce::BubbleComponent& comp, const juce::Point<float>& tip, const juce::Rectangle<float>& _body )
{
	const auto	body = _body.reduced ( 0.5f );

	constexpr	auto	corner = 3.0f;

	// The tip size measures against the unreduced body
	const auto	tipSizeAbove = float ( _body.getY () - tip.getY () ) / 1.5f;
	const auto	tipSizeBelow = float ( tip.getY () - _body.getBottom () ) / 1.5f;

	const auto	tipAbove = tipSizeAbove > 0.1f;

	// No visible tip on either side, nothing to splice
	if ( ! tipAbove && tipSizeBelow <= 0.1f )
	{
		LookAndFeel_V4::drawBubble ( g, comp, tip, _body );
		return;
	}

	auto	p = bubblePath ( body.getWidth (), body.getHeight (), corner, tipAbove ? tipSizeAbove : tipSizeBelow, tipAbove );

	p.applyTransform ( juce::AffineTransform::translation ( body.getTopLeft () ) );

	g.setColour ( comp.findColour ( juce::BubbleComponent::backgroundColourId ) );
	g.fillPath ( p );

	if ( const auto lineW = UI::lineWidth ( UI::lines::slider_bubble ); UI::lines::visible ( lineW ) )
	{
		g.setColour ( comp.findColour ( juce::BubbleComponent::outlineColourId ) );
		g.strokePath ( p, juce::PathStrokeType ( lineW ) );
	}
}
//-------------------------------------------------------------------------------------------------

int GUI_LookAndFeel::getSliderPopupPlacement ( juce::Slider& /*slider*/ )
{
	return juce::BubbleComponent::below;
}
//----------------------------------------------------------------------------------

juce::Label* GUI_LookAndFeel::createSliderTextBox ( juce::Slider& slider )
{
	auto	l = juce::LookAndFeel_V4::createSliderTextBox ( slider );

	UI::setFontRole ( *l, UI::fonts::crt_label );

	return l;
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawLinearSlider ( juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const juce::Slider::SliderStyle style, juce::Slider& slider )
{
	if ( slider.isBar () || style != juce::Slider::SliderStyle::LinearHorizontal )
	{
		juce::LookAndFeel_V4::drawLinearSlider ( g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider );
	}
	else
	{
		const auto	hover = slider.isMouseOverOrDragging ();

		auto&	prop = slider.getProperties ();

		if ( prop.contains ( "choices" ) )
		{
			const int	numChoices = prop[ "choices" ];

			auto	b = slider.getLocalBounds ().toFloat ();
			const auto	choice = int ( slider.getValue () );

			const int	highlightChoice = prop.getWithDefault ( "highlight", -1 );

			g.setFont ( UI::font ( UI::fonts::crt_slider_choice ) );

			// Background
			g.setColour ( UI::getShade ( 0.0f ) );
			g.fillRoundedRectangle ( b, b.getHeight () / 2.0f );

			const auto	choiceWidth = b.getWidth () / numChoices;

			// Render choice texts
			for ( auto i = 0; i < numChoices; ++i )
			{
				const auto	r = b.withX ( i * choiceWidth ).withWidth ( choiceWidth );
				if ( i == choice || i == highlightChoice )
				{
					// Chosen background
					g.setColour ( i == choice ? findColour ( UI::colors::accent ) : UI::getShade ( 0.2f ) );
					g.fillRoundedRectangle ( r.reduced ( 3.0f ), ( r.getHeight () - 6.0f ) / 2.0f );
				}

				g.setColour ( findColour ( UI::colors::text ) );
				g.drawText ( prop[ "choice" + juce::String ( i ) ], r, juce::Justification::centred, false );
			}
		}
		else
		{
			constexpr auto	trackHeight = 4.0f;

			const juce::Point<float>	startPoint ( float ( x ), y + height * 0.5f );

			const auto	sliderCol = findColour ( UI::colors::text );

			// Background track
			{
				const juce::Point<float>	endPoint ( float ( width + x ), startPoint.y );

				juce::Path	backgroundTrack;

				backgroundTrack.startNewSubPath ( startPoint );
				backgroundTrack.lineTo ( endPoint );
				g.setColour ( UI::getShade ( 0.3f ) );
				g.strokePath ( backgroundTrack, { trackHeight, juce::PathStrokeType::curved, juce::PathStrokeType::rounded } );

				// Slider with bi-polar range, draw tick at the 0 position
				if ( slider.getMinimum () < 0.0 )
				{
					juce::Path	middleLine;
					middleLine.startNewSubPath ( float ( width / 2 + x ), 0.0f );
					middleLine.lineTo ( float ( width / 2 + x ), startPoint.y - trackHeight );

					middleLine.startNewSubPath ( float ( width / 2 + x ), startPoint.y + trackHeight );
					middleLine.lineTo ( float ( width / 2 + x ), float ( height ) );

					g.strokePath ( middleLine, { trackHeight / 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded } );
				}
			}

			const juce::Point<float>	maxPoint { sliderPos, ( y + height ) * 0.5f };

			// Value track
			{
				juce::Path	valueTrack;

				valueTrack.startNewSubPath ( startPoint );
				valueTrack.lineTo ( maxPoint );

				g.setColour ( hover ? findColour ( UI::colors::accent ) : sliderCol );
				g.strokePath ( valueTrack, { trackHeight, juce::PathStrokeType::curved, juce::PathStrokeType::rounded } );
			}

			// Thumb
			if ( slider.isMouseOverOrDragging () )
			{
				const auto	thumbHeight = float ( getSliderThumbRadius ( slider ) );

				g.setColour ( sliderCol );
				g.fillRoundedRectangle ( juce::Rectangle<float> ( 3.0f, thumbHeight ).withCentre ( maxPoint ), 1.5f );
			}
		}
	}
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawRasterBars ( juce::Graphics& g, juce::Rectangle<float> b )
{
	static juce::Random	rand;
	static const colodore	colo;
	static auto	c64Palette = colo.generateRGB ( 0, colo.generateYUV ( VIC2_Render::settings::colorStandard::PAL, 60.0f, 100.0f, 60.0f ) );

	static auto	colIdx = 0;
	auto	y = b.getY ();
	do
	{
		auto	h = lime::remap ( rand.nextFloat (), 0.0f, 1.0f, 5.0f, 20.0f );
		if ( rand.nextFloat () < 0.05f )
			h *= 1.5f;

		g.setColour ( juce::Colour ( c64Palette[ colIdx ] ) );
		g.fillRect ( b.withY ( y ).withHeight ( h ) );

		// Do a break somewhere
		const auto	w = rand.nextFloat () * b.getWidth ();
		g.fillRect ( b.withY ( y ).withHeight ( 2.0f ).translated ( w, -1.5f ).withWidth ( b.getWidth () - w ) );

		y += h;

		colIdx = ( colIdx + 1 ) & 15;

	} while ( y < b.getBottom () );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawOutlinedRect ( juce::Graphics& g, const juce::Rectangle<float>& rect, const float radius, const float outline, const juce::Colour outlineCol )
{
	g.fillRoundedRectangle ( rect, radius );

	g.setColour ( outlineCol );
	drawOutline ( g, rect, radius, outline );
}
//-------------------------------------------------------------------------------------------------

void GUI_LookAndFeel::drawOutline ( juce::Graphics& g, const juce::Rectangle<float>& rect, const float radius, const float lineWidth )
{
	if ( ! UI::lines::visible ( lineWidth ) )
		return;

	g.drawRoundedRectangle ( rect.reduced ( lineWidth / 2.0f ), radius - lineWidth / 2.0f, lineWidth );
}
//-------------------------------------------------------------------------------------------------
