#include <JuceHeader.h>

#include "ComponentUtils.h"

//-----------------------------------------------------------------------------

void componentutils::buildComponentMap ( std::unordered_map<juce::String, juce::Component*>& compMap, juce::Component* parent, const juce::String& pName )
{
	// Loop over all children recursivly and build a map of component-names
	for ( auto comp : parent->getChildren () )
	{
		auto	fullName = pName.isNotEmpty () ? pName + "/" + comp->getName () : comp->getName ();

		if ( fullName.endsWithChar ( '/' ) )
			continue;

		// Special handling for Viewports
		if ( auto vp = dynamic_cast<juce::Viewport*> ( comp ) )
		{
			compMap[ fullName ] = comp;

			comp = vp->getViewedComponent ();
			buildComponentMap ( compMap, vp->getViewedComponent (), pName.isNotEmpty () ? pName + "/" + vp->getName () + "/" + comp->getName () : vp->getName () + "/" + comp->getName () );
			continue;
		}

		jassert ( compMap.find ( fullName ) == compMap.end () ); // Duplicate component name!

		compMap[ fullName ] = comp;
		buildComponentMap ( compMap, comp, fullName);
	}
}
//-----------------------------------------------------------------------------
