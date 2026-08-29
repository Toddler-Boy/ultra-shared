#pragma once

#include <JuceHeader.h>

#include <cstdint>
#include <unordered_map>
#include <vector>

//-----------------------------------------------------------------------------

// Read-only access to a zip archive: Data.pak, or the read side under
// ZipFolder. The central directory is parsed once up front; after that every
// access opens its own stream, so readers on any thread share no mutable
// state and need no locks

class PakFile final
{
public:
	struct Entry
	{
		juce::String	path;
		int64_t			headerOffset;
		int64_t			compressedSize;
		int64_t			uncompressedSize;
		uint32_t		crc;
		uint32_t		dosDateTime;	// DOS date in the high word, time in the low
		bool			deflated;
	};

	// Parse the central directory. Any error leaves the pak invalid. The zip
	// may sit appended to a host file (the release exe): all offsets shift by
	// the host's size, which the end-anchored format lets us recover
	bool open ( const juce::File& _pakFile );

	// True when the file ends in a plausible central directory — the appended
	// pak marker. Parsing can still fail afterwards; that's a hard error, not
	// a reason to fall back to loose files
	[[ nodiscard ]] static bool hasZipTail ( const juce::File& file );

	[[ nodiscard ]] bool isValid () const				{	return ! entries.empty ();	}
	[[ nodiscard ]] const juce::File& getFile () const	{	return pakFile;	}
	[[ nodiscard ]] int getNumEntries () const			{	return int ( entries.size () );	}

	// Entries with unreadable compression (anything but stored/deflate)
	[[ nodiscard ]] int getNumUnsupported () const		{	return numUnsupported;	}

	[[ nodiscard ]] const std::vector<Entry>& getEntries () const	{	return entries;	}
	[[ nodiscard ]] const Entry* findEntry ( const juce::String& path ) const	{	return find ( path );	}

	[[ nodiscard ]] bool exists ( const juce::String& path ) const;

	// True when any file lives under the prefix
	[[ nodiscard ]] bool folderExists ( const juce::String& prefix ) const;

	// nullptr when the entry is missing or the pak is unreadable. A deflated
	// entry's stream reads front-to-back; rewinding one re-inflates from the
	// start, so anything seek-heavy belongs in the pak stored, not deflated
	[[ nodiscard ]] std::unique_ptr<juce::InputStream> createStream ( const juce::String& path ) const;

	// The whole entry; empty on a miss or a short read
	[[ nodiscard ]] juce::MemoryBlock load ( const juce::String& path ) const;

	// Paths relative to prefix of the files under it; non-recursive stops at
	// the next '/'. An empty wildcard matches everything
	[[ nodiscard ]] juce::StringArray listFiles ( const juce::String& prefix, const bool recursive, const juce::String& wildcard = {} ) const;

	// Names of the immediate sub-folders under prefix
	[[ nodiscard ]] juce::StringArray listFolders ( const juce::String& prefix ) const;

private:
	[[ nodiscard ]] const Entry* find ( const juce::String& path ) const;

	juce::File			pakFile;
	std::vector<Entry>	entries;
	int					numUnsupported = 0;

	// Case-insensitive, like the file systems the naked layout lives on
	std::unordered_map<std::string, size_t>	lookup;
};
//-----------------------------------------------------------------------------
