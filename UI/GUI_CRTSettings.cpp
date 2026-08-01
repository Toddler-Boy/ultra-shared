#include <JuceHeader.h>

#include "ultra-shared/UI/GUI_CRTSettings.h"

#include "Config/DataSource.h"
#include "Config/FilePaths.h"
#include "ultra-shared/Helpers/ComponentUtils.h"
#include "ultra-shared/UI/Components/GUI_ComboBox.h"
#include "ultra-shared/UI/Components/GUI_CRTSliderIcon.h"
#include "ultra-shared/UI/Components/GUI_CRTSliderLabel.h"
#include "ultra-shared/UI/Components/GUI_Disabler.h"
#include "ultra-shared/UI/Components/GUI_Toggle.h"
#include "ultra-shared/UI/Components/GUI_VIC2_Palette.h"
#include "ultra-shared/UI/Components/GUI_XYPad.h"
#include "ultra-shared/UI/SharedComponentFactory.h"
#include "ultra-shared/UI/UI_Helpers.h"

//-----------------------------------------------------------------------------

GUI_CRTSettings::GUI_CRTSettings ()
	: settingsLayout ( *this, [] ( const juce::String& typeName ) { return sharedComponentFactory ( typeName ); } )
{
	setName ( "settings" );

	settingsViewport.setScrollBarsShown ( true, false );
	settingsViewport.setViewedComponent ( &settingsContent, false );

	addAndMakeVisible ( settingsViewport );

	addChildComponent ( presetSaveDialog );
	presetSaveDialog.onSave = [ this ] ( const juce::String& name ) {	savePresetNamed ( name );	};
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::resized ()
{
	const auto	pos = settingsViewport.getViewPosition ();

	// The viewport's scroll offset, for rows that anchor to content positions
	// (the preset-save body); refreshed again when the dialog opens
	settingsLayout.setConstant ( "viewY", pos.y );

	UI::setLayout ( settingsLayout, {	"UI/layouts/constants.json",
										"UI/layouts/crt-settings.json" } );

	settingsViewport.setViewPosition ( pos );

	if ( settingsComponentMap.empty () )
	{
		componentutils::buildComponentMap ( settingsComponentMap, &settingsContent );
		connectComponents ();
	}
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::timerCallback ( int timerID )
{
	switch ( timerID )
	{
		case 'TV  ':
			stopTimer ( timerID );

			if ( onSettingsChanged )
				onSettingsChanged ();
			updateCRTsettingsUI ();
			break;

		case 'WCAM':
			stopTimer ( timerID );

			if ( onSettingsChanged )
				onSettingsChanged ();
			break;
	}
}
//-----------------------------------------------------------------------------

VIC2_Render::settings GUI_CRTSettings::getVIC2SettingsFromPreferences () const
{
	auto	renderSettings = VIC2_Render::settings {};

	auto getChoiceInt = [ this ] ( const juce::StringArray& choices, const juce::String& name )
	{
		return std::max ( 0, choices.indexOf ( preferences->get<juce::String> ( name ) ) );
	};

	// TV standard (PAL/NTSC)
	{
		const static juce::StringArray	tvSystemChoices { "AUTO", "PAL", "NTSC" };

		auto	intChoiceSystem = getChoiceInt ( tvSystemChoices, "tv/system" );
		if ( intChoiceSystem == 0 )
			intChoiceSystem = tvSystemChoices.indexOf ( autoSystem ? autoSystem () : juce::String (), true );

		renderSettings.standard = VIC2_Render::settings::colorStandard ( intChoiceSystem - 1 );
	}

	// First or revised Luma
	{
		const static juce::StringArray	firstLumaChoices { "AUTO", "FIRST", "REV" };

		auto	intChoiceLuma = getChoiceInt ( firstLumaChoices, "tv/first-luma" );
		if ( intChoiceLuma == 0 )
			intChoiceLuma = ( autoFirstLuma && autoFirstLuma () ) ? 1 : 2;

		renderSettings.firstLuma = intChoiceLuma == 1;
	}

	renderSettings.brightness = preferences->get<int> ( "tv/brightness" );
	renderSettings.contrast = preferences->get<int> ( "tv/contrast" );
	renderSettings.saturation = preferences->get<int> ( "tv/saturation"	);

	renderSettings.raw = ! preferences->get<bool> ( "crt/emulation" );

	return renderSettings;
}
//-----------------------------------------------------------------------------

// A marked pick-list value ("$DATA$/name" / "$USER$/name"); an unmarked legacy
// value counts as factory
static std::pair<juce::String, bool> parseMarkedName ( const juce::String& value )
{
	if ( ! value.startsWith ( "$" ) )
		return { value, false };

	return { value.fromFirstOccurrenceOf ( "/", false, false ),
			 value.startsWith ( filepaths::markerFor ( filepaths::root::user ) ) };
}
//-----------------------------------------------------------------------------

juce::String GUI_CRTSettings::currentOverlayName () const
{
	return parseMarkedName ( preferences->get<juce::String> ( "overlay/bitmap" ) ).first;
}
//-----------------------------------------------------------------------------

lime::CRTEmulation::settings GUI_CRTSettings::getCRTEmulationSettingsFromPreferences () const
{
	auto	set = lime::CRTEmulation::settings {};

	// Overlay settings
	set.overlay = preferences->get<bool> ( "overlay/enabled" );

	// lime only ever sees the plain name; the content loader learns here which
	// variant (factory or user) backs it
	{
		const auto [ name, isUser ] = parseMarkedName ( preferences->get<juce::String> ( "overlay/bitmap" ) );

		set.overlayProfile = name;
		datasource::setActiveUserOverlay ( isUser ? name : juce::String () );
	}

	set.overlayDaytime = preferences->get<int> ( "overlay/daytime" );
	set.overlayBezel = preferences->get<int> ( "overlay/bezel" );
	set.overlayShadow = preferences->get<int> ( "overlay/shadow" );
	set.overlayZoom = preferences->get<int> ( "overlay/zoom" );
	set.overlayDust = preferences->get<int> ( "overlay/dust" );
	set.overlayBloom = preferences->get<int> ( "overlay/bloom" );
	set.overlayChromaticAberration = preferences->get<int> ( "overlay/chromatic-aberration" );
	set.overlayGrain = preferences->get<int> ( "overlay/grain" );

	// TV settings that affect CRT emulation
	set.brightness = preferences->get<int> ( "tv/brightness" );
	set.contrast = preferences->get<int> ( "tv/contrast" );
	set.saturation = preferences->get<int> ( "tv/saturation" );
	set.overscan = preferences->get<int> ( "tv/overscan" );

	// CRT emulation itself
	set.crtEmulation = preferences->get<bool> ( "crt/emulation" );
	set.encJailbars = preferences->get<int> ( "crt/jailbars" );
	set.decNoise = preferences->get<int> ( "crt/noise" );
	set.decSharpening = preferences->get<int> ( "crt/sharpening" );
	set.decLumaBlur = preferences->get<int> ( "crt/luma-blur" );
	set.decChromaBlur = preferences->get<int> ( "crt/chroma-blur" );
	set.decCrosstalk = preferences->get<int> ( "crt/crosstalk" );
	set.decHannover = preferences->get<int> ( "crt/hannover" );
	set.decRainbowing = preferences->get<int> ( "crt/rainbowing" );
	set.decPhaseError = preferences->get<int> ( "crt/drift" );

	set.crtCurve = preferences->get<int> ( "crt/curve" );
	set.crtBleed = preferences->get<int> ( "crt/bleed" );
	set.crtBleedRed = preferences->get<YamlFile::vec2i> ( "crt/bleed-red" );
	set.crtBleedGreen = preferences->get<YamlFile::vec2i> ( "crt/bleed-green" );
	set.crtBleedBlue = preferences->get<YamlFile::vec2i> ( "crt/bleed-blue" );
	set.crtConvergence = preferences->get<int> ( "crt/convergence" );
	set.crtHwave = preferences->get<int> ( "crt/h-wave" );
	set.crtBloomExpansion = preferences->get<int> ( "crt/expansion" );

	set.crtScanlines = preferences->get<int> ( "crt/scanlines" );
	set.crtMask = preferences->get<int> ( "crt/mask" );

	{
		const auto [ name, isUser ] = parseMarkedName ( preferences->get<juce::String> ( "crt/mask-bitmap" ) );

		set.crtMaskBitmap = name;
		datasource::setActiveUserCRTMask ( isUser ? name : juce::String () );
	}

	set.crtPhosphorDecay = preferences->get<int> ( "crt/phosphor-decay" );
	set.crtVignette = preferences->get<int> ( "crt/vignette" );

	set.crtAdjacentExcitation = preferences->get<int> ( "crt/adjacent" );
	set.crtHalation = preferences->get<int> ( "crt/halation" );
	set.crtAmbient = preferences->get<int> ( "crt/ambient" );

	set.crtReflections = preferences->get<int> ( "crt/reflection" );

	set.webcam = preferences->get<bool> ( "webcam/enabled" );
	set.webcamDevice = preferences->get<juce::String> ( "webcam/device" );
	set.webcamBrightness = preferences->get<int> ( "webcam/brightness" );
	set.webcamContrast = preferences->get<int> ( "webcam/contrast" );
	set.webcamSaturation = preferences->get<int> ( "webcam/saturation" );

	return set;
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::updateCRTsettingsUI ()
{
	if ( settingsComponentMap.empty () )
		return;

	const auto	vic2Settings = getVIC2SettingsFromPreferences ();

	auto highlightChoice = [ this ] ( const juce::String& compName, const int choice )
	{
		auto	sld = componentutils::findComponent<juce::Slider> ( compName, settingsComponentMap );
		if ( sld->getProperties ().set ( "highlight", choice ) )
			sld->repaint ();
	};

	highlightChoice ( "tv/system", vic2Settings.standard + 1 );
	highlightChoice ( "tv/first-luma", vic2Settings.firstLuma ? 1 : 2 );

	// Set palette
	auto	palette = componentutils::findComponent<GUI_VIC2_Palette> ( "tv/palette", settingsComponentMap );
	palette->setSettings ( vic2Settings.standard, vic2Settings.brightness, vic2Settings.contrast, vic2Settings.saturation, vic2Settings.firstLuma );
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::updateDisablers ()
{
	auto updateDisabler = [ this ] ( const juce::String& parentName, const juce::String& toggleName )
	{
		auto	disabler = componentutils::findComponent<GUI_Disabler> ( parentName + "/disabler", settingsComponentMap );
		auto	toggle = componentutils::findComponent<juce::ToggleButton> ( parentName + "/" + toggleName, settingsComponentMap );

		disabler->setEnabled ( toggle->getToggleState () );
	};

	updateDisabler ( "overlay", "enabled" );
	updateDisabler ( "crt", "emulation" );
	updateDisabler ( "crt/disabler", "webcam" );
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::connectComponents ()
{
	auto settingsChanged = [ this ]
	{
		if ( onSettingsChanged )
			onSettingsChanged ();

		updatePresetDisplay ();
	};

	auto overlayChanged = [ this ]
	{
		if ( onOverlayChanged )
			onOverlayChanged ();
	};

	auto sliderConnect = [ this, settingsChanged ] ( const juce::String& sldName )
	{
		auto	sld = componentutils::findComponent<GUI_CRTSliderLabel> ( sldName, settingsComponentMap );
		jassert ( sld != nullptr );
		if ( sld == nullptr )
		{
			Z_ERR ( "sliderConnect ( " << sldName << " );" );
			return;
		}

		sld->onValueChange = settingsChanged;
	};

	//
	// Overlay bitmap toggle
	//
	auto	bitmapEnabled = componentutils::findComponent<GUI_Toggle> ( "overlay/enabled", settingsComponentMap );
	bitmapEnabled->onClick = [ bitmapEnabled, settingsChanged, overlayChanged, this ]
	{
		preferences->set ( "overlay/enabled", bitmapEnabled->getToggleState () );
		updateDisablers ();
		settingsChanged ();
		overlayChanged ();
	};

	//
	// Overlay selector (items and selection come from refreshCRTPickLists,
	// which stores marked names - the id indexes overlayMarkedNames)
	//
	{
		auto	overlayBrowser = componentutils::findComponent<juce::ComboBox> ( "overlay/disabler/bitmap", settingsComponentMap );

		overlayBrowser->onChange = [ this, settingsChanged, overlayChanged, overlayBrowser ]
		{
			const auto	id = overlayBrowser->getSelectedId ();
			if ( id <= 0 )
				return;

			preferences->set ( "overlay/bitmap", overlayMarkedNames[ id - 1 ] );
			settingsChanged ();
			overlayChanged ();
		};
	}

	sliderConnect ( "overlay/disabler/daytime" );
	sliderConnect ( "overlay/disabler/bezel" );
	sliderConnect ( "overlay/disabler/shadow" );

	//
	// Zoom
	//
	componentutils::findComponent<GUI_CRTSliderLabel> ( "overlay/disabler/zoom", settingsComponentMap )->onValueChange = [ this, settingsChanged ]
	{
		settingsChanged ();
		if ( onZoomChanged )
			onZoomChanged ();
	};

	sliderConnect ( "overlay/disabler/dust" );
	sliderConnect ( "overlay/disabler/bloom" );
	sliderConnect ( "overlay/disabler/chromatic" );
	sliderConnect ( "overlay/disabler/grain" );

	//
	// Colodore colors
	//
	{
		auto timedSlider = [ this ]
		{
			if ( ! isTimerRunning ( 'TV  ' ) )
			{
				auto choiceToPreference = [ this ] ( const juce::String& pref )
				{
					auto	sld = componentutils::findComponent<juce::Slider> ( pref, settingsComponentMap );
					const juce::String	choice = sld->getProperties ()[ "choice" + juce::String ( int ( sld->getValue () ) ) ];
					preferences->set ( pref, choice );
				};

				auto sliderToPreference = [ this ] ( const juce::String& pref )
				{
					auto	sld = componentutils::findComponent<juce::Slider> ( pref + "/slider", settingsComponentMap );
					preferences->set<int> ( pref, int ( sld->getValue () ) );
				};

				// TV settings
				choiceToPreference ( "tv/system" );
				choiceToPreference ( "tv/first-luma" );

				// Brightness/contrast/saturation
				sliderToPreference ( "tv/brightness" );
				sliderToPreference ( "tv/contrast" );
				sliderToPreference ( "tv/saturation" );

				// Start timer to update CRT values
				startTimer ( 'TV  ', 1000 / 100 );
			}
		};

		auto connectSlider = [ this, &timedSlider ] ( const juce::String& compName )
		{
			componentutils::findComponent<juce::Slider> ( compName, settingsComponentMap )->onValueChange = timedSlider;
		};

		connectSlider ( "tv/system" );
		connectSlider ( "tv/first-luma" );
		connectSlider ( "tv/brightness/slider" );
		connectSlider ( "tv/contrast/slider" );
		connectSlider ( "tv/saturation/slider" );

		updateCRTsettingsUI ();
	}

	//
	// TV overscan
	//
	{
		componentutils::findComponent<GUI_CRTSliderIcon> ( "tv/overscan", settingsComponentMap )->onValueChange = settingsChanged;
	}

	//
	// CRT emulation toggle
	//
	{
		auto	emulation = componentutils::findComponent<GUI_Toggle> ( "crt/emulation", settingsComponentMap );
		emulation->onClick = [ this, settingsChanged, emulation ]
		{
			preferences->set ( "crt/emulation", emulation->getToggleState () );

			updateDisablers ();

			settingsChanged ();
		};
	}

	//
	// CRT emulation parameters
	//
	sliderConnect ( "crt/disabler/jailbars" );

	sliderConnect ( "crt/disabler/noise" );
	sliderConnect ( "crt/disabler/sharpening" );
	sliderConnect ( "crt/disabler/luma-blur" );
	sliderConnect ( "crt/disabler/chroma-blur" );
	sliderConnect ( "crt/disabler/crosstalk" );
	sliderConnect ( "crt/disabler/hannover" );
	sliderConnect ( "crt/disabler/rainbowing" );
	sliderConnect ( "crt/disabler/drift" );

	sliderConnect ( "crt/disabler/curve" );
	sliderConnect ( "crt/disabler/bleed" );
	sliderConnect ( "crt/disabler/h-wave" );
	sliderConnect ( "crt/disabler/convergence" );
	sliderConnect ( "crt/disabler/expansion" );
	sliderConnect ( "crt/disabler/scanlines" );
	sliderConnect ( "crt/disabler/mask" );
	sliderConnect ( "crt/disabler/phosphor" );
	sliderConnect ( "crt/disabler/vignette" );

	sliderConnect ( "crt/disabler/adjacent" );
	sliderConnect ( "crt/disabler/halation" );
	sliderConnect ( "crt/disabler/ambient" );

	sliderConnect ( "crt/disabler/reflection" );

	// CRT Bleed
	{
		auto	bleedRed = componentutils::findComponent<GUI_XYPad> ( "crt/disabler/bleed-red", settingsComponentMap );
		auto	bleedGreen = componentutils::findComponent<GUI_XYPad> ( "crt/disabler/bleed-green", settingsComponentMap );
		auto	bleedBlue = componentutils::findComponent<GUI_XYPad> ( "crt/disabler/bleed-blue", settingsComponentMap );

		bleedRed->onValueChange = settingsChanged;
		bleedGreen->onValueChange = settingsChanged;
		bleedBlue->onValueChange = settingsChanged;
	}

	//
	// Webcam Reflections
	//
	{
		auto	reflectionSource = componentutils::findComponent<GUI_Toggle> ( "crt/disabler/webcam", settingsComponentMap );
		reflectionSource->onClick = [ this, settingsChanged, reflectionSource ]
		{
			preferences->set ( "webcam/enabled", reflectionSource->getToggleState () );

			updateDisablers ();
			settingsChanged ();
		};
	}

	//
	// Webcam settings
	//
	{
		auto timedSlider60 = [ this ]
		{
			if ( ! isTimerRunning ( 'WCAM' ) )
				startTimer ( 'WCAM', 1000 / 60 );
		};

		componentutils::findComponent<GUI_CRTSliderIcon> ( "crt/disabler/disabler/brightness", settingsComponentMap )->onValueChange = timedSlider60;
		componentutils::findComponent<GUI_CRTSliderIcon> ( "crt/disabler/disabler/contrast", settingsComponentMap )->onValueChange = timedSlider60;
		componentutils::findComponent<GUI_CRTSliderIcon> ( "crt/disabler/disabler/saturation", settingsComponentMap )->onValueChange = timedSlider60;

		// Device selector (items come from refreshWebcamDevices)
		auto	deviceBrowser = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/disabler/device", settingsComponentMap );

		deviceBrowser->onChange = [ this, settingsChanged, deviceBrowser ]
		{
			if ( deviceBrowser->getSelectedId () <= 0 )
				return;

			preferences->set ( "webcam/device", deviceBrowser->getText () );
			settingsChanged ();
		};

		refreshWebcamDevices ();
	}

	//
	// CRT preset selector (items from refreshCRTPickLists, the id indexes
	// presetMarkedNames; one past the end = the developer save action)
	//
	{
		auto	presetBrowser = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/preset", settingsComponentMap );

		presetBrowser->onChange = [ this, settingsChanged, overlayChanged, presetBrowser ]
		{
			const auto	id = presetBrowser->getSelectedId ();
			if ( id <= 0 )
				return;

			// One past the end: the save action. The combo snaps back to the
			// stored preset while the dialog asks for a name; a selected user
			// preset pre-fills it for easy overwriting
			if ( id > presetMarkedNames.size () )
			{
				updatePresetDisplay ();

				// Re-layout so viewY carries the current scroll position
				resized ();

				const auto	stored = preferences->get<juce::String> ( "crt/preset" );
				const auto	userMarker = filepaths::markerFor ( filepaths::root::user ) + "/";

				presetSaveDialog.show ( stored.startsWith ( userMarker ) ? stored.fromFirstOccurrenceOf ( "/", false, false ) : juce::String () );
				return;
			}

			const auto&	marked = presetMarkedNames[ id - 1 ];

			preferences->set ( "crt/preset", marked );
			currentPreset.load ( marked );
			currentPreset.applyTo ( *preferences );

			restorePresetScopeWidgets ();
			refreshCRTPickLists ();	// re-selects the mask pattern from the preference

			settingsChanged ();
			overlayChanged ();	// the preset may have swapped the mask bitmap
		};
	}

	//
	// CRT-Mask-bitmap selector (same marked-name scheme as the overlays)
	//
	{
		auto	crtMaskBrowser = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/mask-bitmap", settingsComponentMap );

		crtMaskBrowser->onChange = [ this, settingsChanged, overlayChanged, crtMaskBrowser ]
		{
			const auto	id = crtMaskBrowser->getSelectedId ();
			if ( id <= 0 )
				return;

			preferences->set ( "crt/mask-bitmap", maskMarkedNames[ id - 1 ] );
			settingsChanged ();
			overlayChanged ();
		};
	}

	refreshCRTPickLists ();

	settingsChanged ();
	overlayChanged ();

	//
	// Set initial values
	//
	if ( onZoomChanged )
		onZoomChanged ();

	updateDisablers ();
}
//-----------------------------------------------------------------------------

// Factory names come from datasource directly (never the merged loader view),
// user names from the real user folders - pak mode only, dev mode has no
// loader to serve them
static juce::StringArray factoryOverlayNames ()
{
	juce::StringArray	out;

	for ( const auto& name : datasource::listFolders ( "CRTEmulation/Overlays" ) )
		if ( datasource::exists ( "CRTEmulation/Overlays/" + name + "/profile.yml" ) )
			out.add ( name );

	out.sortNatural ();

	return out;
}
//-----------------------------------------------------------------------------

static juce::StringArray userOverlayNames ()
{
	juce::StringArray	out;

	if ( ! datasource::isPak () )
		return out;

	if ( const auto root = filepaths::getUserOverlaysPath (); root != juce::File () )
		for ( const auto& f : root.findChildFiles ( juce::File::findDirectories | juce::File::ignoreHiddenFiles, false ) )
			if ( f.getChildFile ( "profile.yml" ).existsAsFile () )
				out.add ( f.getFileName () );

	out.sortNatural ();

	return out;
}
//-----------------------------------------------------------------------------

static juce::StringArray factoryMaskNames ()
{
	juce::StringArray	out;

	for ( const auto& name : datasource::listFiles ( "CRTEmulation/CRT Masks", false, "*.png" ) )
		out.add ( name.upToLastOccurrenceOf ( ".", false, false ) );

	out.sortNatural ();

	return out;
}
//-----------------------------------------------------------------------------

static juce::StringArray userMaskNames ()
{
	juce::StringArray	out;

	if ( ! datasource::isPak () )
		return out;

	if ( const auto root = filepaths::getUserCRTMasksPath (); root != juce::File () )
		for ( const auto& f : root.findChildFiles ( juce::File::findFiles | juce::File::ignoreHiddenFiles, false, "*.png" ) )
			out.add ( f.getFileNameWithoutExtension () );

	out.sortNatural ();

	return out;
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::refreshWebcamDevices ()
{
	// Camera APIs can block on misbehaving (virtual) drivers, so the
	// enumeration runs off the message thread; the combo fills a beat later
	juce::Thread::launch ( juce::Thread::Priority::low, [ safe = juce::Component::SafePointer ( this ) ]
	{
		juce::StringArray	names;
		for ( const auto& n : lime::Webcam::getDeviceNames () )
			names.add ( juce::String ( n ) );

		// The engine opens device 0 when no name is stored yet
		const auto	firstDevice = names.isEmpty () ? juce::String () : names[ 0 ];

		// Display order only; selection is by name, the sr layer resolves it
		// against its own enumeration order
		names.sortNatural ();

		juce::MessageManager::callAsync ( [ safe, names, firstDevice ]
		{
			if ( safe == nullptr )
				return;

			auto	box = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/disabler/device", safe->settingsComponentMap );

			box->clear ( juce::dontSendNotification );
			box->addItemList ( names, 1 );

			// A blank preference becomes the camera the engine picks anyway,
			// so the user sees which one is in use
			if ( safe->preferences->get<juce::String> ( "webcam/device" ).isEmpty () && firstDevice.isNotEmpty () )
				safe->preferences->set ( "webcam/device", firstDevice );

			// A stored but unplugged device shows as bare text, no selection
			box->setText ( safe->preferences->get<juce::String> ( "webcam/device" ), juce::dontSendNotification );
		} );
	} );
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::refreshCRTPickLists ()
{
	const auto	dataMarker = filepaths::markerFor ( filepaths::root::data ) + "/";
	const auto	userMarker = filepaths::markerFor ( filepaths::root::user ) + "/";

	// Factory group first, then the user's own behind a separator, marked with
	// an icon - the theme selector's scheme. Items carry plain names, the id
	// indexes the marked form the preference stores
	auto build = [ & ] ( const char* componentName, juce::StringArray& markedNames,
						 const juce::StringArray& factory, const juce::StringArray& user, const juce::String& prefKey )
	{
		auto	box = componentutils::findComponent<juce::ComboBox> ( componentName, settingsComponentMap );

		box->clear ( juce::dontSendNotification );
		markedNames.clear ();

		auto&	menu = *box->getRootMenu ();

		for ( const auto& name : factory )
		{
			markedNames.add ( dataMarker + name );

			juce::PopupMenu::Item	item ( name );
			item.itemID = markedNames.size ();
			item.setImage ( UI::getMenuIcon ( icons->get ( "crt-factory" ) ) );
			menu.addItem ( item );
		}

		if ( ! factory.isEmpty () && ! user.isEmpty () )
			menu.addSeparator ();

		for ( const auto& name : user )
		{
			markedNames.add ( userMarker + name );

			juce::PopupMenu::Item	item ( name );
			item.itemID = markedNames.size ();
			item.setImage ( UI::getMenuIcon ( icons->get ( "crt-user" ) ) );
			menu.addItem ( item );
		}

		// Legacy unmarked preference counts as factory; an unknown stored name
		// selects nothing
		auto	stored = preferences->get<juce::String> ( prefKey );
		if ( ! stored.startsWith ( "$" ) )
			stored = dataMarker + stored;

		box->setSelectedId ( markedNames.indexOf ( stored ) + 1, juce::dontSendNotification );
	};

	build ( "overlay/disabler/bitmap", overlayMarkedNames, factoryOverlayNames (), userOverlayNames (), "overlay/bitmap" );
	build ( "crt/disabler/mask-bitmap", maskMarkedNames, factoryMaskNames (), userMaskNames (), "crt/mask-bitmap" );

	//
	// CRT presets: the same factory/user menu scheme; re-parse the stored
	// preset too, its file may be what just changed
	//
	{
		auto	box = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/preset", settingsComponentMap );

		box->clear ( juce::dontSendNotification );
		presetMarkedNames = crtpresets::listPresets ();

		auto&	menu = *box->getRootMenu ();
		auto	factorySeen = false;

		for ( const auto& marked : presetMarkedNames )
		{
			const auto	isUser = marked.startsWith ( userMarker );

			if ( isUser && factorySeen )
			{
				menu.addSeparator ();
				factorySeen = false;
			}
			else if ( ! isUser )
			{
				factorySeen = true;
			}

			juce::PopupMenu::Item	item ( marked.fromFirstOccurrenceOf ( "/", false, false ) );
			item.itemID = presetMarkedNames.indexOf ( marked ) + 1;
			item.setImage ( UI::getMenuIcon ( icons->get ( isUser ? "crt-user" : "crt-factory" ) ) );
			menu.addItem ( item );
		}

		{
			menu.addSeparator ();

			juce::PopupMenu::Item	item ( strings->get ( "crt/settings/crt/preset-save" ) );
			item.itemID = presetMarkedNames.size () + 1;
			menu.addItem ( item );
		}

		currentPreset.load ( preferences->get<juce::String> ( "crt/preset" ) );
		updatePresetDisplay ();
	}
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::restorePresetScopeWidgets ()
{
	static const char* const	sliders[] =
	{
		"noise", "sharpening", "luma-blur", "chroma-blur", "crosstalk", "hannover", "rainbowing", "drift",
		"curve", "bleed", "convergence", "h-wave", "expansion",
		"scanlines", "mask", "phosphor", "vignette", "adjacent", "halation", "ambient", "reflection"
	};

	for ( const auto* name : sliders )
		componentutils::findComponent<GUI_CRTSliderLabel> ( juce::String ( "crt/disabler/" ) + name, settingsComponentMap )->restorePreference ();

	static const char* const	pads[] = { "bleed-red", "bleed-green", "bleed-blue" };

	for ( const auto* name : pads )
		componentutils::findComponent<GUI_XYPad> ( juce::String ( "crt/disabler/" ) + name, settingsComponentMap )->restorePreference ();
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::updatePresetDisplay ()
{
	if ( settingsComponentMap.empty () )
		return;

	auto	box = componentutils::findComponent<juce::ComboBox> ( "crt/disabler/preset", settingsComponentMap );

	const auto	index = presetMarkedNames.indexOf ( currentPreset.markedName () );

	if ( index >= 0 && currentPreset.matches ( *preferences ) )
	{
		if ( box->getSelectedId () != index + 1 )
			box->setSelectedId ( index + 1, juce::dontSendNotification );

		return;
	}

	if ( const auto& custom = strings->get ( "crt/settings/crt/preset-custom" ); box->getText () != custom )
		box->setText ( custom, juce::dontSendNotification );
}
//-----------------------------------------------------------------------------

void GUI_CRTSettings::savePresetNamed ( const juce::String& name )
{
	// The name doubles as the file name
	const auto	legal = juce::File::createLegalFileName ( name ).trim ();
	if ( legal.isEmpty () )
		return;

	auto commit = [ this, legal ]
	{
		const auto	saved = crtpresets::saveCurrentValues ( *preferences, legal );

		if ( saved.isNotEmpty () )
			preferences->set ( "crt/preset", saved );

		presetSaveDialog.dismiss ();
		refreshCRTPickLists ();
	};

	if ( filepaths::getUserCRTPresetsPath ().getChildFile ( legal + ".yml" ).existsAsFile () )
	{
		const auto	options = juce::MessageBoxOptions ()
								.withIconType ( juce::MessageBoxIconType::QuestionIcon )
								.withTitle ( strings->get ( "crt/settings/crt/preset-replace-title" ) )
								.withMessage ( strings->get ( "crt/settings/crt/preset-replace" ).replace ( "{}", legal ) )
								.withButton ( strings->get ( "crt/settings/crt/preset-save-action" ) )
								.withButton ( strings->get ( "crt/settings/crt/preset-cancel" ) )
								.withAssociatedComponent ( this );

		// 0 = the first button; the dialog stays up behind a declined overwrite
		juce::NativeMessageBox::showAsync ( options, [ commit ] ( const int result )
		{
			if ( result == 0 )
				commit ();
		} );

		return;
	}

	commit ();
}
//-----------------------------------------------------------------------------
