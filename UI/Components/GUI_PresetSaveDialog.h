#pragma once

#include <JuceHeader.h>

#include "ultra-shared/Resources/Strings.h"
#include "ultra-shared/UI/Components/GUI_Label.h"
#include "ultra-shared/UI/Components/GUI_SVG_Button.h"
#include "ultra-shared/UI/Components/GUI_SolidButton.h"
#include "ultra-shared/UI/Components/GUI_TextEditorEx.h"

//-----------------------------------------------------------------------------

// Asks for a preset name: a click-swallowing veil with a drop-shadowed "body"
// box. The whole tree ("preset-save" down to the buttons) is laid out by the
// OWNER's layout json (crt-settings.json), so tuning hot-reloads like any
// other row. The owner validates and writes via onSave and decides when to
// dismiss, so the dialog can stay up through an overwrite confirmation

class GUI_PresetSaveDialog final : public juce::Component
{
public:
	GUI_PresetSaveDialog ();

	// juce::Component
	void paint ( juce::Graphics& g ) override;
	bool keyPressed ( const juce::KeyPress& key ) override;
	void visibilityChanged () override;

	// this
	void show ( const juce::String& suggestedName );
	void dismiss ();

	// The typed name, already trimmed and non-empty
	std::function<void ( const juce::String& )>	onSave;

private:
	// Registered with the Desktop only while the dialog is up: any click
	// outside the body — other panels, the OpenGL screen — dismisses. Native
	// message boxes don't route through here, so the overwrite confirm is safe
	class OutsideClickListener final : private juce::MouseListener
	{
	public:
		explicit OutsideClickListener ( GUI_PresetSaveDialog& _dialog ) : dialog ( _dialog ) {}
		~OutsideClickListener () override	{	stop ();	}

		void start ()	{	juce::Desktop::getInstance ().addGlobalMouseListener ( this );	}
		void stop ()	{	juce::Desktop::getInstance ().removeGlobalMouseListener ( this );	}

	private:
		void mouseDown ( const juce::MouseEvent& event ) override
		{
			if ( ! dialog.body.getScreenBounds ().contains ( event.getScreenPosition () ) )
				dialog.dismiss ();
		}

		GUI_PresetSaveDialog&	dialog;
	};

	void saveClicked ();

	OutsideClickListener	outsideClick { *this };

	juce::Component		body { "body" };

	GUI_DynamicLabel	title { "crt/settings/crt/preset-save-title", UI::fonts::dialog_title };
	GUI_TextEditorEx	nameEditor { "name" };
	GUI_SolidButton		save { "save", "crt/settings/crt/preset-save-action" };
	GUI_SVG_Button		close { "close", { "about/close" } };

	melatonin::DropShadow	shadow { 12.0 };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_PresetSaveDialog )
};
//-----------------------------------------------------------------------------
