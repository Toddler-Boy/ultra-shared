#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

// Builds the layout-JSON component types every app shares (the generic widgets
// and the CRT-settings set); the app's componentFactory handles its own types
// first and falls back to this. Returns nullptr for types it doesn't know
[[ nodiscard ]] std::pair<juce::Component*, bool> sharedComponentFactory ( const juce::String& compType, juce::StringArray typeParts );

// Full "type(arg, arg)" form for layouts that only use shared types (the
// CRT-settings panel); unknown types log and return nullptr
[[ nodiscard ]] std::pair<juce::Component*, bool> sharedComponentFactory ( const juce::String& typeName );
//-----------------------------------------------------------------------------
