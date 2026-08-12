#pragma once

#include <JuceHeader.h>

//-----------------------------------------------------------------------------

namespace fileutils
{
	// Crash-safe whole-file replacement via a visible sibling temp file. Juce's own
	// replaceWith* use a dot-prefixed temp, which SMB shares on Linux NAS hosts
	// store as hidden, leaving the target file permanently hidden after the rename.
	// Failures are logged here; the result is for callers with their own unwinding.
	inline bool replaceFile ( const juce::File& file, const void* data, const size_t numBytes )
	{
		// Replacing with nothing needs no temporary (appendData would never
		// create it anyway), truncating the live file does the job
		if ( numBytes == 0 )
		{
			if ( file.deleteFile () && file.create () )
				return true;

			Z_ERR ( "Could not save " << file.getFullPathName () );
			return false;
		}

		juce::TemporaryFile	temp ( file );

		if ( temp.getFile ().appendData ( data, numBytes ) && temp.overwriteTargetFileWithTemporary () )
			return true;

		Z_ERR ( "Could not save " << file.getFullPathName () );
		return false;
	}

	inline bool replaceFile ( const juce::File& file, const juce::String& text )
	{
		juce::TemporaryFile	temp ( file );

		if ( temp.getFile ().appendText ( text, false, false, "\r\n" ) && temp.overwriteTargetFileWithTemporary () )
			return true;

		Z_ERR ( "Could not save " << file.getFullPathName () );
		return false;
	}
}
//-----------------------------------------------------------------------------
