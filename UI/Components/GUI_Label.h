#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Resources/Strings.h"
#include "UI/ui-colors.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

class GUI_Label : public juce::Component
{
public:
	GUI_Label ( const juce::String& _text, const float fSize, const int weight, const int colId = UI::colors::text )
		: juce::Component ( "label" )
		, text ( _text )
		, font ( UI::fontSized ( fSize, weight ) )
		, colorId ( colId )
	{
		setInterceptsMouseClicks ( false, false );
	}

	GUI_Label ( const juce::String& _text, const UI::fonts::Role role, const int colId = UI::colors::text )
		: juce::Component ( "label" )
		, text ( _text )
		, font ( UI::font ( role ) )
		, fontRole ( role )
		, colorId ( colId )
	{
		setInterceptsMouseClicks ( false, false );
	}

	// juce::Component

	// A themed role re-resolves on theme reload; explicitly set fonts stay
	void lookAndFeelChanged () override
	{
		if ( fontRole != UI::fonts::count )
			font = UI::font ( fontRole );
	}

	void paint ( juce::Graphics& g ) override
	{
		g.setFont ( font );
		g.setColour ( findColour ( colorId ) );
		g.drawText ( text, getLocalBounds ().toFloat (), justification, false );
	}

	// this
	void setText ( const juce::String& _text )
	{
		if ( text == _text )
			return;

		text = _text;
		repaint ();
	}

	[[ nodiscard ]] juce::String& getText ()
	{
		return text;
	}

	void setFont ( const juce::Font& _font )
	{
		font = _font;
		repaint ();
	}

	void setFontRole ( const UI::fonts::Role role )
	{
		fontRole = role;
		font = UI::font ( role );
		repaint ();
	}

	void setColor ( const int col )
	{
		colorId = col;
		repaint ();
	}

	void setJustification ( const juce::Justification _just )
	{
		justification = _just;
		repaint ();
	}

protected:
	juce::String		text;
	juce::Font			font { juce::FontOptions {} };
	UI::fonts::Role		fontRole = UI::fonts::count;	// count = no themed role
	int					colorId = UI::colors::text;
	juce::Justification justification = juce::Justification::centredLeft;

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_Label )
};
//-----------------------------------------------------------------------------

class GUI_DynamicLabel final : public GUI_Label
{
public:
	GUI_DynamicLabel ( const juce::String& _text, const float fSize, const int weight, const int colId = UI::colors::text )
		: GUI_Label ( _text, fSize, weight, colId )
	{
	}

	GUI_DynamicLabel ( const juce::String& _text, const UI::fonts::Role role, const int colId = UI::colors::text )
		: GUI_Label ( _text, role, colId )
	{
	}

	// GUI_Label
	void paint ( juce::Graphics& g ) override
	{
		g.setFont ( font );
		g.setColour ( findColour ( colorId ) );
		g.drawText ( strings->get ( text ), getLocalBounds ().toFloat (), justification, false );
	}

private:
	juce::SharedResourcePointer<Strings>    strings;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_DynamicLabel )
};
//-----------------------------------------------------------------------------
