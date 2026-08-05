#pragma once

#include <JuceHeader.h>

#include "ultra-shared/UI/Components/GUI_SVG_Button.h"
#include "ultra-shared/UI/Components/GUI_TextEditorEx.h"

//----------------------------------------------------------------------------------

class GUI_SearchBar final : public juce::Component, public juce::TextEditor::Listener
{
public:
	GUI_SearchBar ();

	// juce::Component
	void resized () override;
	void paint ( juce::Graphics& g ) override;
	void mouseDown ( const juce::MouseEvent& event ) override;
	void focusGained ( juce::Component::FocusChangeType cause ) override;
	void focusLost ( juce::Component::FocusChangeType cause ) override;
	void lookAndFeelChanged () override;

	// juce::TextEditor::Listener
	void textEditorTextChanged ( juce::TextEditor& ) override;

	// This
	void updateClearButton ();
	[[ nodiscard ]] GUI_TextEditorEx& getTextEditor ()	{	return textEditor;	}
	std::function<void ()>	onTextChange;

private:
	GUI_TextEditorEx	textEditor { "editor" };
	GUI_SVG_Button		clearSearch { "delete", { "search_bar/clear" } };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_SearchBar )
};
//----------------------------------------------------------------------------------
