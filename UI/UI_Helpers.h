#pragma once

#include <JuceHeader.h>

#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include "UI/ui-corners.h"
#include "UI/ui-fonts.h"
#include "UI/ui-lines.h"
#include "UI/ui-paddings.h"

//-----------------------------------------------------------------------------

namespace UI
{
	extern juce::Colour	startColor;
	extern juce::Colour	endColor;

	// Preset blend values for getShade()
	namespace shades
	{
		constexpr auto	hover = 1.0f / 12.0f;
		constexpr auto	selected = 1.0f / 6.0f;
	}

	void setShades ( const juce::Colour col1, const juce::Colour col2 ) noexcept;
	[[ nodiscard ]] juce::Colour getShade ( const float blend ) noexcept;
	[[ nodiscard ]] juce::Colour getAverageColor ( const juce::Image& img, const float bright, const float satMul, const float satDiv );
	[[ nodiscard ]] juce::Colour getColorFromName ( const juce::String& name, const float brightness = 0.25f );
	[[ nodiscard ]] float easeInOutQuad ( float t );
	[[ nodiscard ]] juce::Colour getColorWithPerceivedBrightness ( const juce::Colour col, const float perceivedBrightness ) noexcept;

	// Point size = the em size, comparable 1:1 with a CSS px value. For
	// deliberate one-offs and role derivations - themed text uses font ( role )
	[[ nodiscard ]] juce::Font fontSized ( const float points, const int weight = 500 );

	// A role would silently convert to a point size - font ( role ) is the way
	juce::Font fontSized ( UI::fonts::Role, int = 0 ) = delete;

	// Themed font role (theme "fonts:" section), resolved on every call so a
	// theme hot-reload takes effect on the next repaint
	[[ nodiscard ]] juce::Font font ( UI::fonts::Role role );

	// The raw themed definition, for deriving related fonts from a role
	[[ nodiscard ]] UI::fonts::Def fontDef ( UI::fonts::Role role );

	// Binds a juce::Label to a themed role: getLabelFont () re-resolves it on
	// every paint, so theme switches and hot reloads always land
	void setFontRole ( juce::Label& label, UI::fonts::Role role );

	// Themed corner radius (theme "corners:" section), resolved on every call
	// so a theme hot-reload takes effect on the next repaint. Passing the
	// rectangle being drawn (or its shorter side) clamps the radius to half
	// the shorter side, so a theme value like 1000 yields a pill shape (JUCE
	// alone clamps per axis and would draw an oval instead)
	[[ nodiscard ]] float corner ( UI::corners::Role role, float shorterSide = 0.0f );
	[[ nodiscard ]] float corner ( UI::corners::Role role, const juce::Rectangle<float>& r );

	// Themed line width (theme "lines:" section), resolved on every call so a
	// theme hot-reload takes effect on the next repaint. 0 hides the line
	[[ nodiscard ]] float lineWidth ( UI::lines::Role role );

	// The rect with the themed padding subtracted from each side (negative
	// theme values grow it), resolved on every call so hot-reloads land
	[[ nodiscard ]] juce::Rectangle<float> padded ( const juce::Rectangle<float>& r, UI::paddings::Role role );

	// The raw themed per-side values, for deriving sizes from a padding
	[[ nodiscard ]] UI::paddings::Def paddingDef ( UI::paddings::Role role );

	// CSS shorthand: "all", "tb lr", "top lr bottom" or "top right bottom left";
	// negatives allowed. nullopt for anything else
	[[ nodiscard ]] std::optional<UI::paddings::Def> parsePadding ( const juce::String& value );
	[[ nodiscard ]] juce::Font monoFont ( const float points, const int weight );

	[[ nodiscard ]] std::unique_ptr<juce::Drawable> getMenuIcon ( const juce::String& name );
	[[ nodiscard ]] juce::PopupMenu::Item newMenuItem ( const juce::String& name, const juce::String& icon, std::function<void ()> func );
	[[ nodiscard ]] juce::PopupMenu::Item newDangerousMenuItem ( const juce::String& name, const juce::String& icon, std::function<void ()> func );

	// Popup-menu boilerplate: build with newPopupMenu ( owner ), fill, then
	// show with one of these; the deletion check on owner is always baked in
	[[ nodiscard ]] juce::PopupMenu newPopupMenu ( juce::Component& owner );
	void showMenuAtMouse ( juce::PopupMenu& m, juce::Component& owner );
	void showMenuAtButton ( juce::PopupMenu& m, juce::Component& owner, juce::Component& anchor );

	[[ nodiscard ]] std::pair<std::unique_ptr<juce::Drawable>, int> getSVG ( const juce::String& svgName );

	// Feeds data-relative layout files to gin: the naked layout hands over real
	// files so gin's own watcher hot-reloads them, the pak hands over content
	void setLayout ( gin::LayoutSupport& layout, const juce::StringArray& paths );
	[[ nodiscard ]] juce::Path& getScaledPath ( const juce::String& resourceName, juce::Rectangle<float> rect, juce::RectanglePlacement placement = 0, float padding = 0.0f );
	[[ nodiscard ]] juce::Path& getScaledPathWithSize ( const juce::String& resourceName, juce::Rectangle<float> rect, juce::RectanglePlacement placement = 0, float padding = 0.0f );

	void repaintCell ( juce::TableListBox* tlb, const int rowNumber, const int columnId );
	void repaintColumn ( juce::TableListBox* tlb, const int columnId );
}
//-----------------------------------------------------------------------------
