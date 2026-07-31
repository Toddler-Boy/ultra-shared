#pragma once

#include <JuceHeader.h>

#include <unordered_map>
#include <vector>

//-----------------------------------------------------------------------------

namespace componentutils
{
	void buildComponentMap ( std::unordered_map<juce::String, juce::Component*>& compMap, juce::Component* parent, const juce::String& name = "" );

	template<typename T>
	void getChildrenOfClass ( juce::Component* parent, std::vector<T*>& comps )
	{
		// Loop over all children recursivly and build a vector of components of class T
		for ( auto comp : parent->getChildren () )
		{
			if ( auto tComp = dynamic_cast<T*> ( comp ) )
				comps.push_back ( tComp );

			getChildrenOfClass<T> ( comp, comps );
		}
	}

	template<typename T>
	[[ nodiscard ]] T* findComponent ( const juce::String& name, const std::unordered_map<juce::String, juce::Component*>& compMap )
	{
		auto	it = compMap.find ( name );

		// The layout JSON is data, so a name can legitimately be absent
		jassert ( it != compMap.end () );
		if ( it == compMap.end () )
		{
			Z_ERR ( "Component not found: " << name );
			return nullptr;
		}

		return dynamic_cast<T*> ( it->second );
	}
}
//-----------------------------------------------------------------------------
