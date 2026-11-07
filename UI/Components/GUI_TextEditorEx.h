#pragma once

#include <JuceHeader.h>
#include <optional>

//-------------------------------------------------------------------------------------------------

class GUI_TextEditorEx final : public juce::TextEditor
{
public:
	GUI_TextEditorEx (const juce::String& componentName = juce::String (), juce::juce_wchar passwordCharacter = 0);

	enum ColourIds
	{
		// Set on an editor: its backgroundColourId while it has the keyboard focus, the resting one back on focus loss
		focusedBackgroundColourId = 0x1000290,
	};

	// juce::Component
	void mouseDoubleClick (const juce::MouseEvent& event) override;
	void mouseUp (const juce::MouseEvent& event) override;

	void mouseDown ( const juce::MouseEvent& e ) override;

	void focusGained ( juce::Component::FocusChangeType cause) override;
	void focusLost ( juce::Component::FocusChangeType cause) override;

	// juce::TextEditor
	void returnPressed () override;
	void escapePressed () override;
	juce::Rectangle<int> getCaretRectangleForCharIndex ( int index ) const override;

	// This
	void disableClickOnlyFocus ();
	void enableSingleClick ();
	void selectAllText ();

	std::function<juce::Rectangle<int> ()> onGetScreenBounds;
	std::function<void ()>	onReturnPressed;
	std::function<void ()>	onEscapePressed;

private:
	// This
	void handBackFocus ();

	// Armed only while the editor has focus: its one job is spotting the
	// outside click that should take focus away
	class ClickoutSideListener : private juce::MouseListener
	{
	public:
		ClickoutSideListener ( GUI_TextEditorEx& e );
		~ClickoutSideListener () override;

		void start ();
		void stop ();

	private:
		void mouseDown ( const juce::MouseEvent& e ) override;

		GUI_TextEditorEx& editor;
	};

	ClickoutSideListener clickoutSideListener { *this };

	bool			wantsDoubleClick = true;
	bool			clickOnlyFocus = true;
	bool			hasFocus = false;
	bool			mouseDownInEditor = false;

	// The background before focusGained swapped in focusedBackgroundColourId
	std::optional<juce::Colour>	restingBackground;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GUI_TextEditorEx)
};
//-------------------------------------------------------------------------------------------------
