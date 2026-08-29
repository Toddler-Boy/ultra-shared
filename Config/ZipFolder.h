#pragma once

#include <JuceHeader.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "ultra-shared/Config/PakFile.h"

//-----------------------------------------------------------------------------

// A zip archive presented as a mutable folder: reads go to the archive (a
// lock-free PakFile), mutations stage in memory until commit() atomically
// rewrites the file. Not thread-safe while changes are pending

class ZipFolder final
{
public:
	bool open ( const juce::File& zipFile );

	[[ nodiscard ]] bool isValid () const				{	return pak.isValid ();	}
	[[ nodiscard ]] const juce::File& getFile () const	{	return pak.getFile ();	}

	[[ nodiscard ]] bool exists ( const juce::String& path ) const;

	// True when any file lives under the prefix
	[[ nodiscard ]] bool folderExists ( const juce::String& prefix ) const;

	// The whole entry; empty on a miss or a short read
	[[ nodiscard ]] juce::MemoryBlock load ( const juce::String& path ) const;

	// nullptr on a miss. Deflated archive entries stream front-to-back only
	[[ nodiscard ]] std::unique_ptr<juce::InputStream> createStream ( const juce::String& path ) const;

	// Paths relative to prefix of the files under it; non-recursive stops at
	// the next '/'. An empty wildcard matches everything
	[[ nodiscard ]] juce::StringArray listFiles ( const juce::String& prefix, const bool recursive, const juce::String& wildcard = {} ) const;

	// Names of the immediate sub-folders under prefix
	[[ nodiscard ]] juce::StringArray listFolders ( const juce::String& prefix ) const;

	// Add or replace; the data is copied
	void writeFile ( const juce::String& path, const void* data, const size_t numBytes );
	void writeFile ( const juce::String& path, const juce::MemoryBlock& data )	{	writeFile ( path, data.getData (), data.getSize () );	}

	// False when the source is missing; renaming onto an existing path
	// replaces it
	bool rename ( const juce::String& from, const juce::String& to );

	// False when the path is missing
	bool remove ( const juce::String& path );

	// Removes every file under the prefix; false when there were none
	bool removeFolder ( const juce::String& prefix );

	[[ nodiscard ]] bool hasPendingChanges () const		{	return ! overlay.empty ();	}
	void discardPendingChanges ()						{	overlay.clear ();	}

	// Atomically rewrites the archive with all staged changes and reopens; on
	// failure the file and the pending changes stay as they were
	bool commit ();

	//-----------------------------------------------------------------------------

	// Streams a new zip front to back. Entries deflate at the given level
	// (store when deflate doesn't pay); 4 GB size cap, entry counts past 64K
	// switch to zip64 end records
	class Writer final
	{
	public:
		explicit Writer ( juce::OutputStream& _out ) : out ( _out ) {}

		bool addFile ( const juce::String& path, const void* data, const size_t numBytes, const juce::Time modTime, const int compressionLevel = 1 );

		// Byte-copy of an already-compressed entry, positioned at its data
		bool addRaw ( const juce::String& path, const bool deflated, const uint32_t crc, const uint32_t dosDateTime, const int64_t compressedSize, const int64_t uncompressedSize, juce::InputStream& compressedData );

		// Central directory + end records; the Writer is spent afterwards
		bool finish ();

	private:
		struct Written
		{
			juce::String	path;
			int64_t			headerOffset;
			int64_t			compressedSize;
			int64_t			uncompressedSize;
			uint32_t		crc;
			uint32_t		dosDateTime;
			bool			deflated;
			bool			utf8;
		};

		bool writeLocalHeader ( Written& w );

		juce::OutputStream&		out;
		std::vector<Written>	written;
		bool					failed = false;
	};

private:
	// New content, a rename of an archive entry (renamedFrom), or a deletion
	// tombstone
	struct Staged
	{
		juce::String		path;	// current name, original case
		juce::MemoryBlock	data;
		juce::String		renamedFrom;
		bool				deleted = false;
	};

	[[ nodiscard ]] const Staged* findStaged ( const juce::String& path ) const;
	void eraseOrTombstone ( const std::string& key, const juce::String& path );

	PakFile									pak;
	std::unordered_map<std::string, Staged>	overlay;	// key = lowered path
};
//-----------------------------------------------------------------------------
