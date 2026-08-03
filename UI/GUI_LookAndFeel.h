#pragma once

#include <JuceHeader.h>

#include "ultra-shared/UI/UI_Helpers.h"

class MipMap;

// The shared look of both apps; an app derives when it draws things of its
// own (ultraSID adds the player progress pieces)
class GUI_LookAndFeel : public juce::LookAndFeel_V4, public juce::MouseListener
{
public:
	GUI_LookAndFeel ();
	~GUI_LookAndFeel () override;

	[[ nodiscard ]] juce::Font fontWithHeight ( const float height )
	{
		return juce::Font ( defaultFont.withHeight ( height) );
	}

	[[ nodiscard ]] juce::Font font ( const float height, const int weight )
	{
		return juce::Font ( fontWeighted ( weight ).withHeight ( height ) );
	}

	// SemiBold, the weight the mono face is used at throughout
	[[ nodiscard ]] juce::Font monoFontWithHeight ( const float height )
	{
		return juce::Font ( monoWeighted ( 600 ).withHeight ( height ) );
	}

	// Mono variant of fontPoints (). Always goes through a "wght" clone: the
	// subset's default instance renders wrong
	[[ nodiscard ]] juce::Font monoFontPoints ( const float points, const int weight )
	{
		const auto	f = juce::Font ( monoWeighted ( weight ).withPointHeight ( points ) );

		return f.withHeight ( f.getHeight () );
	}

	// Point size = the em size, like a CSS px value (JUCE heights run ~1.3x larger).
	// The result carries a resolved JUCE height: parts of JUCE (the TextEditor
	// among them) mishandle fonts that only bring a point size
	[[ nodiscard ]] juce::Font fontPoints ( const float points, const int weight )
	{
		const auto	f = juce::Font ( fontWeighted ( weight ).withPointHeight ( points ) );

		return f.withHeight ( f.getHeight () );
	}

	//
	// Standard JUCE look and feel functions
	//
	juce::Font getSliderPopupFont ( juce::Slider& ) override	{	return fontWithHeight ( 17.0f );	}
	juce::Font getPopupMenuFont () override;
	juce::Font getTextButtonFont ( juce::TextButton&, int /*buttonHeight*/ ) override
	{
		// Same role drawButtonText () paints with, so measurements agree
		return UI::font ( UI::fonts::settings_location_button );
	}

	juce::Font getComboBoxFont ( juce::ComboBox& box ) override;

	// A "fontRole" property (see UI::setFontRole) binds a label to the theme:
	// resolved on every paint, so theme switches and hot reloads always land
	juce::Font getLabelFont ( juce::Label& label ) override
	{
		const auto	role = int ( label.getProperties ().getWithDefault ( "fontRole", -1 ) );

		return role >= 0 ? UI::font ( UI::fonts::Role ( role ) ) : label.getFont ();
	}

	//
	// Window decorations (titlebar, border, frame, etc.)
	//
	juce::Button* createDocumentWindowButton ( int ) override;
	void positionDocumentWindowButtons ( juce::DocumentWindow&, int titleBarX, int titleBarY, int titleBarW, int titleBarH, juce::Button* minimiseButton, juce::Button* maximiseButton, juce::Button* closeButton, bool positionTitleBarButtonsOnLeft ) override;
 	void drawResizableWindowBorder ( juce::Graphics&, int w, int h, const juce::BorderSize<int>& border, juce::ResizableWindow& window ) override;
	void drawResizableFrame ( juce::Graphics& g, int w, int h, const juce::BorderSize<int>& border ) override;
	void drawDocumentWindowTitleBar ( juce::DocumentWindow& window, juce::Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const juce::Image* icon, bool drawTitleTextOnLeft ) override;
	void mouseEnter ( const juce::MouseEvent& e ) override;
	void mouseExit ( const juce::MouseEvent& e ) override;

	//
	// Scrollbar
	//
	int getDefaultScrollbarWidth () override;
	int getMinimumScrollbarThumbSize ( juce::ScrollBar& ) override;
	void drawScrollbar ( juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown ) override;

	//
	// Tooltip
	//
	[[ nodiscard ]] juce::TextLayout layoutTooltipText ( const juce::String& text ) noexcept;
	juce::Rectangle<int> getTooltipBounds ( const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea ) override;
	void drawTooltip ( juce::Graphics&, const juce::String& text, int width, int height ) override;

 	std::unique_ptr<juce::DropShadower> createDropShadowerForComponent ( juce::Component& comp ) override
 	{
		if ( dynamic_cast<juce::TooltipWindow*> ( &comp ) )
			return nullptr;

		return juce::LookAndFeel_V4::createDropShadowerForComponent ( comp );
 	}

	//
	// PopupMenu
	//
	int getMenuWindowFlags () override { return 0; }	// This disable the default shadow (completely square and ugly)
	void preparePopupMenuWindow ( juce::Component& newWindow ) override;
	void drawPopupMenuBackground ( juce::Graphics& g, int width, int height ) override;
	void drawPopupMenuItem ( juce::Graphics& g, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon, const juce::Colour* textColour ) override;
	void drawPopupMenuSectionHeader ( juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& sectionName ) override;
	void drawPopupMenuColumnSeparatorWithOptions ( juce::Graphics& g, const juce::Rectangle<int>& area, const juce::PopupMenu::Options& ) override;
	int getPopupMenuColumnSeparatorWidthWithOptions ( const juce::PopupMenu::Options&) override;
	int getPopupMenuBorderSizeWithOptions ( const juce::PopupMenu::Options& ) override;
	void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

	//
	// Misc
	//
	void drawBubble ( juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip, const juce::Rectangle<float>& body ) override;
	int getSliderPopupPlacement ( juce::Slider& ) override;

	void positionComboBoxText ( juce::ComboBox& box, juce::Label& label ) override;

	void drawLabel ( juce::Graphics&, juce::Label& ) override;
	void drawComboBox ( juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& ) override;

	void drawButtonBackground ( juce::Graphics& g, juce::Button& b, const juce::Colour& backgroundColour, bool isHover, bool isDown ) override;
	void drawButtonText ( juce::Graphics&, juce::TextButton&, bool isHover, bool isDown ) override;

	void fillTextEditorBackground ( juce::Graphics&, int width, int height, juce::TextEditor& ) override;
	void drawTextEditorOutline ( juce::Graphics&, int width, int height, juce::TextEditor& ) override;

	void drawTableHeaderBackground ( juce::Graphics& g, juce::TableHeaderComponent& th ) override;
	void drawTableHeaderColumn ( juce::Graphics& g, juce::TableHeaderComponent& th, const juce::String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags ) override;

	juce::Label* createSliderTextBox ( juce::Slider& ) override;
	void drawLinearSlider ( juce::Graphics&, int x, int y, int width, int height,
							float sliderPos, float minSliderPos, float maxSliderPos,
							const juce::Slider::SliderStyle, juce::Slider& ) override;

	//
	// Drawing helpers
	//
	static void drawRasterBars ( juce::Graphics& g, juce::Rectangle<float> b );
	static void drawOutlinedRect ( juce::Graphics& g, const juce::Rectangle<float>& rect, const float radius, const float outline, const juce::Colour outlineCol );

	// Rounded outline kept fully inside rect (radius describes rect itself, the
	// inset is derived). Hidden widths (theme value 0) skip the draw entirely
	static void drawOutline ( juce::Graphics& g, const juce::Rectangle<float>& rect, const float radius, const float lineWidth );

private:
	// On-demand per-weight clones of the variable fonts, cached for the app's
	// lifetime. The weight is the true "wght" axis value (Figtree covers
	// 300-900), any in-between value works
	[[ nodiscard ]] const juce::FontOptions& fontWeighted ( int weight );
	[[ nodiscard ]] const juce::FontOptions& monoWeighted ( int weight );

	juce::Typeface::Ptr							figtreeBase;
	std::unordered_map<int, juce::FontOptions>	fontCache;
	std::unordered_map<int, juce::FontOptions>	monoFontCache;

	juce::FontOptions	defaultFont;

	juce::Typeface::Ptr	monoBase;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_LookAndFeel )
};
//----------------------------------------------------------------------------------
