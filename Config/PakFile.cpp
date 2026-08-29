#include "ultra-shared/Config/PakFile.h"

#include <algorithm>

//-----------------------------------------------------------------------------

namespace
{
	// The little-endian readers work on any alignment
	[[ nodiscard ]] uint32_t u16 ( const uint8_t* p )	{	return uint32_t ( p[ 0 ] ) | uint32_t ( p[ 1 ] ) << 8;	}
	[[ nodiscard ]] uint32_t u32 ( const uint8_t* p )	{	return uint32_t ( p[ 0 ] ) | uint32_t ( p[ 1 ] ) << 8 | uint32_t ( p[ 2 ] ) << 16 | uint32_t ( p[ 3 ] ) << 24;	}
	[[ nodiscard ]] uint64_t u64 ( const uint8_t* p )	{	return uint64_t ( u32 ( p ) ) | uint64_t ( u32 ( p + 4 ) ) << 32;	}

	[[ nodiscard ]] std::string lowerKey ( const juce::String& path )
	{
		return path.replaceCharacter ( '\\', '/' ).toLowerCase ().toStdString ();
	}

	// "UI/themes" and "UI/themes/" both mean the folder
	[[ nodiscard ]] juce::String normalizedPrefix ( const juce::String& prefix )
	{
		if ( prefix.isEmpty () || prefix.endsWithChar ( '/' ) )
			return prefix;

		return prefix + "/";
	}
	//-----------------------------------------------------------------------------

	struct EOCD
	{
		int64_t		numEntries = 0;
		int64_t		cdSize = 0;
		int64_t		cdOffset = 0;
		int64_t		base = 0;	// Bytes in front of the zip: the host exe when appended
	};

	// End-of-central-directory record: 22 fixed bytes plus an optional comment
	// of up to 64K, found by scanning backwards for its signature. A candidate
	// only counts when its geometry fits the file, so stray signature bytes in
	// code or in the Authenticode blob a signed exe carries behind the real
	// record get skipped. 0xFFFF/0xFFFFFFFF are zip64 sentinels: the real
	// numbers then sit in the zip64 record in front of the locator
	[[ nodiscard ]] bool findEOCD ( juce::FileInputStream& in, EOCD& out )
	{
		const auto	fileSize = in.getTotalLength ();

		// Worst case: zip64 record + locator (76 bytes) in front of an EOCD
		// carrying a maximum-length comment
		const auto	tailSize = std::min ( fileSize, int64_t ( 65633 ) );
		if ( tailSize < 22 )
			return false;

		juce::MemoryBlock	tail;
		in.setPosition ( fileSize - tailSize );
		if ( juce::int64 ( in.readIntoMemoryBlock ( tail, tailSize ) ) != tailSize )
			return false;

		const auto	t = static_cast<const uint8_t*> ( tail.getData () );

		for ( auto i = int ( tailSize ) - 22; i >= 0; --i )
		{
			if ( u32 ( t + i ) != 0x06054b50 )
				continue;

			const auto	numEntries = int64_t ( u16 ( t + i + 10 ) );
			const auto	cdSize = int64_t ( u32 ( t + i + 12 ) );
			const auto	cdOffset = int64_t ( u32 ( t + i + 16 ) );

			if ( numEntries == 0xFFFF || cdSize == 0xFFFFFFFF || cdOffset == 0xFFFFFFFF )
			{
				// zip64: the 20-byte locator and the 56-byte record precede
				// the classic one
				if ( i < 76 || u32 ( t + i - 20 ) != 0x07064b50 || u32 ( t + i - 76 ) != 0x06064b50 )
					continue;

				const auto	z = t + i - 76;
				const auto	numEntries64 = int64_t ( u64 ( z + 32 ) );
				const auto	cdSize64 = int64_t ( u64 ( z + 40 ) );
				const auto	cdOffset64 = int64_t ( u64 ( z + 48 ) );

				// The central directory ends where the zip64 record starts
				const auto	base = ( fileSize - tailSize + i - 76 ) - cdSize64 - cdOffset64;
				if ( base < 0 )
					continue;

				out = { numEntries64, cdSize64, cdOffset64, base };
				return true;
			}

			// The central directory ends right where the record starts; its
			// distance to the zip-relative offset it claims is the host size
			const auto	base = ( fileSize - tailSize + i ) - cdSize - cdOffset;
			if ( base < 0 )
				continue;

			out = { numEntries, cdSize, cdOffset, base };
			return true;
		}

		return false;
	}
}
//-----------------------------------------------------------------------------

bool PakFile::open ( const juce::File& _pakFile )
{
	pakFile = _pakFile;
	entries.clear ();
	lookup.clear ();
	numUnsupported = 0;

	juce::FileInputStream	in ( pakFile );
	if ( ! in.openedOk () )
		return false;

	EOCD	eocd;
	if ( ! findEOCD ( in, eocd ) )
	{
		Z_ERR ( "No end-of-central-directory in " << pakFile.getFullPathName () );
		return false;
	}

	const auto	numEntries = eocd.numEntries;
	const auto	cdSize = eocd.cdSize;

	//
	// Central directory: 46 fixed bytes per entry plus name/extra/comment.
	// Every offset the zip stores is relative to its own start, hence + base
	//
	juce::MemoryBlock	cd;
	in.setPosition ( eocd.base + eocd.cdOffset );
	if ( juce::int64 ( in.readIntoMemoryBlock ( cd, cdSize ) ) != cdSize )
		return false;

	const auto	d = static_cast<const uint8_t*> ( cd.getData () );

	entries.reserve ( size_t ( numEntries ) );
	lookup.reserve ( size_t ( numEntries ) );

	juce::int64	pos = 0;
	for ( int64_t i = 0; i < numEntries; ++i )
	{
		if ( pos + 46 > cdSize || u32 ( d + pos ) != 0x02014b50 )
		{
			Z_ERR ( "Central directory truncated in " << pakFile.getFullPathName () );
			entries.clear ();
			lookup.clear ();
			return false;
		}

		const auto	method = int ( u16 ( d + pos + 10 ) );
		const auto	dosDateTime = u32 ( d + pos + 12 );
		const auto	crc = u32 ( d + pos + 16 );
		auto		compressedSize = int64_t ( u32 ( d + pos + 20 ) );
		auto		uncompressedSize = int64_t ( u32 ( d + pos + 24 ) );
		const auto	nameLen = int ( u16 ( d + pos + 28 ) );
		const auto	extraLen = int ( u16 ( d + pos + 30 ) );
		const auto	commentLen = int ( u16 ( d + pos + 32 ) );
		auto		headerOffset = int64_t ( u32 ( d + pos + 42 ) );

		const auto	path = juce::String::fromUTF8 ( reinterpret_cast<const char*> ( d + pos + 46 ), nameLen );

		// Sentinel fields carry their real number in the zip64 extra field:
		// 64-bit values in fixed order, present per sentinel
		if ( compressedSize == 0xFFFFFFFF || uncompressedSize == 0xFFFFFFFF || headerOffset == 0xFFFFFFFF )
		{
			auto		e = d + pos + 46 + nameLen;
			const auto	extraEnd = e + extraLen;

			while ( e + 4 <= extraEnd )
			{
				const auto	fieldSize = int ( u16 ( e + 2 ) );
				auto		v = e + 4;
				const auto	fieldEnd = v + fieldSize;

				if ( fieldEnd > extraEnd )
					break;

				if ( u16 ( e ) == 0x0001 )
				{
					if ( uncompressedSize == 0xFFFFFFFF && v + 8 <= fieldEnd )	{	uncompressedSize = int64_t ( u64 ( v ) );	v += 8;	}
					if ( compressedSize == 0xFFFFFFFF && v + 8 <= fieldEnd )	{	compressedSize = int64_t ( u64 ( v ) );		v += 8;	}
					if ( headerOffset == 0xFFFFFFFF && v + 8 <= fieldEnd )		{	headerOffset = int64_t ( u64 ( v ) );	}
					break;
				}

				e = fieldEnd;
			}
		}

		pos += 46 + nameLen + extraLen + commentLen;

		// Folders carry no data; anything but stored/deflate can't be read
		if ( path.endsWithChar ( '/' ) )
			continue;

		if ( method != 0 && method != 8 )
		{
			Z_ERR ( "Unsupported compression on " << path << " in " << pakFile.getFullPathName () );
			++numUnsupported;
			continue;
		}

		lookup[ lowerKey ( path ) ] = entries.size ();
		entries.push_back ( { path, eocd.base + headerOffset, compressedSize, uncompressedSize, crc, dosDateTime, method == 8 } );
	}

	return isValid ();
}
//-----------------------------------------------------------------------------

bool PakFile::hasZipTail ( const juce::File& file )
{
	juce::FileInputStream	in ( file );
	EOCD					eocd;

	return in.openedOk () && findEOCD ( in, eocd );
}
//-----------------------------------------------------------------------------

const PakFile::Entry* PakFile::find ( const juce::String& path ) const
{
	const auto	it = lookup.find ( lowerKey ( path ) );

	return it == lookup.end () ? nullptr : &entries[ it->second ];
}
//-----------------------------------------------------------------------------

bool PakFile::exists ( const juce::String& path ) const
{
	return find ( path ) != nullptr;
}
//-----------------------------------------------------------------------------

bool PakFile::folderExists ( const juce::String& prefix ) const
{
	const auto	pre = normalizedPrefix ( prefix );

	for ( const auto& e : entries )
		if ( e.path.startsWithIgnoreCase ( pre ) )
			return true;

	return false;
}
//-----------------------------------------------------------------------------

std::unique_ptr<juce::InputStream> PakFile::createStream ( const juce::String& path ) const
{
	const auto	e = find ( path );
	if ( e == nullptr )
		return nullptr;

	auto	in = std::make_unique<juce::FileInputStream> ( pakFile );
	if ( ! in->openedOk () )
	{
		Z_ERR ( "Cannot open " << pakFile.getFullPathName () );
		return nullptr;
	}

	// The local header repeats name/extra with its own lengths, so the data
	// offset comes from it, not from the central directory
	uint8_t	lh[ 30 ];
	in->setPosition ( e->headerOffset );
	if ( in->read ( lh, 30 ) != 30 || u32 ( lh ) != 0x04034b50 )
	{
		Z_ERR ( "Bad local header for " << e->path << " in " << pakFile.getFullPathName () );
		return nullptr;
	}

	const auto	dataOffset = e->headerOffset + 30 + juce::int64 ( u16 ( lh + 26 ) ) + juce::int64 ( u16 ( lh + 28 ) );

	auto	sub = std::make_unique<juce::SubregionStream> ( in.release (), dataOffset, e->compressedSize, true );

	if ( ! e->deflated )
		return sub;

	return std::make_unique<juce::GZIPDecompressorInputStream> ( sub.release (), true, juce::GZIPDecompressorInputStream::deflateFormat, e->uncompressedSize );
}
//-----------------------------------------------------------------------------

juce::MemoryBlock PakFile::load ( const juce::String& path ) const
{
	const auto	e = find ( path );

	auto	stream = createStream ( path );
	if ( stream == nullptr )
		return {};

	juce::MemoryBlock	mb;
	stream->readIntoMemoryBlock ( mb );

	if ( juce::int64 ( mb.getSize () ) != e->uncompressedSize )
	{
		Z_ERR ( "Short read of " << path << " from " << pakFile.getFullPathName () );
		return {};
	}

	return mb;
}
//-----------------------------------------------------------------------------

juce::StringArray PakFile::listFiles ( const juce::String& prefix, const bool recursive, const juce::String& wildcard ) const
{
	const auto	pre = normalizedPrefix ( prefix );
	const auto	preLen = pre.length ();

	juce::StringArray	ret;

	for ( const auto& e : entries )
	{
		if ( ! e.path.startsWithIgnoreCase ( pre ) )
			continue;

		const auto	relative = e.path.substring ( preLen );

		if ( ! recursive && relative.containsChar ( '/' ) )
			continue;

		if ( wildcard.isNotEmpty () && ! relative.fromLastOccurrenceOf ( "/", false, false ).matchesWildcard ( wildcard, true ) )
			continue;

		ret.add ( relative );
	}

	return ret;
}
//-----------------------------------------------------------------------------

juce::StringArray PakFile::listFolders ( const juce::String& prefix ) const
{
	const auto	pre = normalizedPrefix ( prefix );
	const auto	preLen = pre.length ();

	juce::StringArray	ret;

	for ( const auto& e : entries )
	{
		if ( ! e.path.startsWithIgnoreCase ( pre ) )
			continue;

		const auto	relative = e.path.substring ( preLen );

		if ( ! relative.containsChar ( '/' ) )
			continue;

		ret.addIfNotAlreadyThere ( relative.upToFirstOccurrenceOf ( "/", false, false ) );
	}

	return ret;
}
//-----------------------------------------------------------------------------
