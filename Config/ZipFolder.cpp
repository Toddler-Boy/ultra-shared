#include "ultra-shared/Config/ZipFolder.h"

#include <algorithm>
#include <array>
#include <cstdint>

//-----------------------------------------------------------------------------

namespace
{
	// The little-endian readers work on any alignment
	[[ nodiscard ]] uint32_t u16 ( const uint8_t* p )	{	return uint32_t ( p[ 0 ] ) | uint32_t ( p[ 1 ] ) << 8;	}
	[[ nodiscard ]] uint32_t u32 ( const uint8_t* p )	{	return uint32_t ( p[ 0 ] ) | uint32_t ( p[ 1 ] ) << 8 | uint32_t ( p[ 2 ] ) << 16 | uint32_t ( p[ 3 ] ) << 24;	}

	[[ nodiscard ]] std::string lowerKey ( const juce::String& path )
	{
		return path.replaceCharacter ( '\\', '/' ).toLowerCase ().toStdString ();
	}

	[[ nodiscard ]] juce::String normalizedPath ( const juce::String& path )
	{
		return path.replaceCharacter ( '\\', '/' );
	}

	// "UI/themes" and "UI/themes/" both mean the folder
	[[ nodiscard ]] juce::String normalizedPrefix ( const juce::String& prefix )
	{
		if ( prefix.isEmpty () || prefix.endsWithChar ( '/' ) )
			return prefix;

		return prefix + "/";
	}

	// Standard reflected CRC-32
	[[ nodiscard ]] uint32_t crc32of ( const void* data, const size_t numBytes )
	{
		static const auto	table = []
		{
			std::array<uint32_t, 256>	t {};

			for ( uint32_t n = 0; n < 256; ++n )
			{
				auto	c = n;
				for ( auto k = 0; k < 8; ++k )
					c = ( c & 1 ) != 0 ? 0xEDB88320u ^ ( c >> 1 ) : c >> 1;
				t[ n ] = c;
			}

			return t;
		} ();

		auto		crc = 0xFFFFFFFFu;
		const auto	p = static_cast<const uint8_t*> ( data );

		for ( size_t i = 0; i < numBytes; ++i )
			crc = table[ ( crc ^ p[ i ] ) & 0xFF ] ^ ( crc >> 8 );

		return crc ^ 0xFFFFFFFFu;
	}

	[[ nodiscard ]] uint32_t toDosDateTime ( const juce::Time t )
	{
		const auto	date = uint32_t ( ( std::max ( 0, t.getYear () - 1980 ) << 9 ) | ( ( t.getMonth () + 1 ) << 5 ) | t.getDayOfMonth () );
		const auto	time = uint32_t ( ( t.getHours () << 11 ) | ( t.getMinutes () << 5 ) | ( t.getSeconds () >> 1 ) );

		return ( date << 16 ) | time;
	}

	[[ nodiscard ]] bool isAscii ( const char* utf8, const int numBytes )
	{
		for ( auto i = 0; i < numBytes; ++i )
			if ( uint8_t ( utf8[ i ] ) >= 0x80 )
				return false;

		return true;
	}
}
//-----------------------------------------------------------------------------

bool ZipFolder::Writer::writeLocalHeader ( Written& w )
{
	w.headerOffset = out.getPosition ();

	if ( w.headerOffset >= 0xFFFFFFFF || w.compressedSize >= 0xFFFFFFFF || w.uncompressedSize >= 0xFFFFFFFF )
	{
		Z_ERR ( "Zip past 4 GB at " << w.path );
		failed = true;
		return false;
	}

	const auto*	utf8 = w.path.toRawUTF8 ();
	const auto	nameLen = int ( w.path.getNumBytesAsUTF8 () );

	// The utf-8 name flag only goes on names that need it
	w.utf8 = ! isAscii ( utf8, nameLen );

	out.writeInt ( 0x04034b50 );
	out.writeShort ( 20 );								// version needed
	out.writeShort ( short ( w.utf8 ? 0x0800 : 0 ) );
	out.writeShort ( short ( w.deflated ? 8 : 0 ) );
	out.writeInt ( int ( w.dosDateTime ) );
	out.writeInt ( int ( w.crc ) );
	out.writeInt ( int ( w.compressedSize ) );
	out.writeInt ( int ( w.uncompressedSize ) );
	out.writeShort ( short ( nameLen ) );
	out.writeShort ( 0 );								// no extra field

	return out.write ( utf8, size_t ( nameLen ) );
}
//-----------------------------------------------------------------------------

bool ZipFolder::Writer::addFile ( const juce::String& path, const void* data, const size_t numBytes, const juce::Time modTime, const int compressionLevel )
{
	if ( failed )
		return false;

	juce::MemoryOutputStream	deflatedData;

	if ( compressionLevel > 0 && numBytes > 0 )
	{
		juce::GZIPCompressorOutputStream	gz ( deflatedData, compressionLevel, juce::GZIPCompressorOutputStream::windowBitsRaw );
		gz.write ( data, numBytes );
		gz.flush ();
	}

	// Store when deflate doesn't pay
	const auto	deflated = compressionLevel > 0 && numBytes > 0 && deflatedData.getDataSize () < numBytes;

	Written	w;
	w.path = normalizedPath ( path );
	w.compressedSize = int64_t ( deflated ? deflatedData.getDataSize () : numBytes );
	w.uncompressedSize = int64_t ( numBytes );
	w.crc = crc32of ( data, numBytes );
	w.dosDateTime = toDosDateTime ( modTime );
	w.deflated = deflated;

	if ( ! writeLocalHeader ( w ) )
		return false;

	if ( w.compressedSize > 0 && ! out.write ( deflated ? deflatedData.getData () : data, size_t ( w.compressedSize ) ) )
	{
		failed = true;
		return false;
	}

	written.push_back ( std::move ( w ) );
	return true;
}
//-----------------------------------------------------------------------------

bool ZipFolder::Writer::addRaw ( const juce::String& path, const bool deflated, const uint32_t crc, const uint32_t dosDateTime, const int64_t compressedSize, const int64_t uncompressedSize, juce::InputStream& compressedData )
{
	if ( failed )
		return false;

	Written	w;
	w.path = normalizedPath ( path );
	w.compressedSize = compressedSize;
	w.uncompressedSize = uncompressedSize;
	w.crc = crc;
	w.dosDateTime = dosDateTime;
	w.deflated = deflated;

	if ( ! writeLocalHeader ( w ) )
		return false;

	if ( out.writeFromInputStream ( compressedData, compressedSize ) != compressedSize )
	{
		Z_ERR ( "Short copy of " << w.path );
		failed = true;
		return false;
	}

	written.push_back ( std::move ( w ) );
	return true;
}
//-----------------------------------------------------------------------------

bool ZipFolder::Writer::finish ()
{
	if ( failed )
		return false;

	const auto	cdOffset = out.getPosition ();

	for ( const auto& w : written )
	{
		const auto*	utf8 = w.path.toRawUTF8 ();
		const auto	nameLen = int ( w.path.getNumBytesAsUTF8 () );

		out.writeInt ( 0x02014b50 );
		out.writeShort ( 20 );							// version made by
		out.writeShort ( 20 );							// version needed
		out.writeShort ( short ( w.utf8 ? 0x0800 : 0 ) );
		out.writeShort ( short ( w.deflated ? 8 : 0 ) );
		out.writeInt ( int ( w.dosDateTime ) );
		out.writeInt ( int ( w.crc ) );
		out.writeInt ( int ( w.compressedSize ) );
		out.writeInt ( int ( w.uncompressedSize ) );
		out.writeShort ( short ( nameLen ) );
		out.writeShort ( 0 );							// extra
		out.writeShort ( 0 );							// comment
		out.writeShort ( 0 );							// disk
		out.writeShort ( 0 );							// internal attributes
		out.writeInt ( 0 );								// external attributes
		out.writeInt ( int ( w.headerOffset ) );
		out.write ( utf8, size_t ( nameLen ) );
	}

	const auto	cdSize = out.getPosition () - cdOffset;
	const auto	numEntries = int64_t ( written.size () );

	// Past 64K entries the real count moves into a zip64 record + locator and
	// the classic record keeps the sentinel; writeLocalHeader's 4 GB cap
	// keeps everything else 32-bit
	if ( numEntries >= 0xFFFF )
	{
		const auto	zip64Pos = out.getPosition ();

		out.writeInt ( 0x06064b50 );
		out.writeInt64 ( 44 );							// record size past this field
		out.writeShort ( 45 );							// version made by
		out.writeShort ( 45 );							// version needed
		out.writeInt ( 0 );								// disk
		out.writeInt ( 0 );								// central directory disk
		out.writeInt64 ( numEntries );
		out.writeInt64 ( numEntries );
		out.writeInt64 ( cdSize );
		out.writeInt64 ( cdOffset );

		out.writeInt ( 0x07064b50 );					// locator
		out.writeInt ( 0 );
		out.writeInt64 ( zip64Pos );
		out.writeInt ( 1 );
	}

	const auto	classicCount = short ( std::min ( numEntries, int64_t ( 0xFFFF ) ) );

	out.writeInt ( 0x06054b50 );
	out.writeShort ( 0 );
	out.writeShort ( 0 );
	out.writeShort ( classicCount );
	out.writeShort ( classicCount );
	out.writeInt ( int ( cdSize ) );
	out.writeInt ( int ( cdOffset ) );
	out.writeShort ( 0 );								// no comment

	return true;
}
//-----------------------------------------------------------------------------

bool ZipFolder::open ( const juce::File& zipFile )
{
	overlay.clear ();

	return pak.open ( zipFile );
}
//-----------------------------------------------------------------------------

const ZipFolder::Staged* ZipFolder::findStaged ( const juce::String& path ) const
{
	const auto	it = overlay.find ( lowerKey ( path ) );

	return it == overlay.end () ? nullptr : &it->second;
}
//-----------------------------------------------------------------------------

bool ZipFolder::exists ( const juce::String& path ) const
{
	if ( const auto* s = findStaged ( path ) )
		return ! s->deleted;

	return pak.exists ( path );
}
//-----------------------------------------------------------------------------

bool ZipFolder::folderExists ( const juce::String& prefix ) const
{
	if ( overlay.empty () )
		return pak.folderExists ( prefix );

	const auto	pre = normalizedPrefix ( prefix );

	for ( const auto& relative : pak.listFiles ( pre, true ) )
		if ( ! overlay.contains ( lowerKey ( pre + relative ) ) )
			return true;

	for ( const auto& [ key, s ] : overlay )
		if ( ! s.deleted && s.path.startsWithIgnoreCase ( pre ) )
			return true;

	return false;
}
//-----------------------------------------------------------------------------

juce::MemoryBlock ZipFolder::load ( const juce::String& path ) const
{
	if ( const auto* s = findStaged ( path ) )
	{
		if ( s->deleted )
			return {};

		if ( s->renamedFrom.isNotEmpty () )
			return pak.load ( s->renamedFrom );

		return s->data;
	}

	return pak.load ( path );
}
//-----------------------------------------------------------------------------

std::unique_ptr<juce::InputStream> ZipFolder::createStream ( const juce::String& path ) const
{
	if ( const auto* s = findStaged ( path ) )
	{
		if ( s->deleted )
			return nullptr;

		if ( s->renamedFrom.isNotEmpty () )
			return pak.createStream ( s->renamedFrom );

		// Copies, so the stream survives further staging
		return std::make_unique<juce::MemoryInputStream> ( s->data, true );
	}

	return pak.createStream ( path );
}
//-----------------------------------------------------------------------------

juce::StringArray ZipFolder::listFiles ( const juce::String& prefix, const bool recursive, const juce::String& wildcard ) const
{
	const auto	pre = normalizedPrefix ( prefix );

	juce::StringArray	ret;

	// Staged state hides same-named archive entries; the overlay pass adds
	// the live ones
	for ( const auto& relative : pak.listFiles ( pre, recursive, wildcard ) )
		if ( ! overlay.contains ( lowerKey ( pre + relative ) ) )
			ret.add ( relative );

	for ( const auto& [ key, s ] : overlay )
	{
		if ( s.deleted || ! s.path.startsWithIgnoreCase ( pre ) )
			continue;

		const auto	relative = s.path.substring ( pre.length () );

		if ( ! recursive && relative.containsChar ( '/' ) )
			continue;

		if ( wildcard.isNotEmpty () && ! relative.fromLastOccurrenceOf ( "/", false, false ).matchesWildcard ( wildcard, true ) )
			continue;

		ret.add ( relative );
	}

	return ret;
}
//-----------------------------------------------------------------------------

juce::StringArray ZipFolder::listFolders ( const juce::String& prefix ) const
{
	const auto	pre = normalizedPrefix ( prefix );

	juce::StringArray	ret;

	for ( const auto& relative : pak.listFiles ( pre, true ) )
		if ( ! overlay.contains ( lowerKey ( pre + relative ) ) && relative.containsChar ( '/' ) )
			ret.addIfNotAlreadyThere ( relative.upToFirstOccurrenceOf ( "/", false, false ) );

	for ( const auto& [ key, s ] : overlay )
	{
		if ( s.deleted || ! s.path.startsWithIgnoreCase ( pre ) )
			continue;

		const auto	relative = s.path.substring ( pre.length () );

		if ( relative.containsChar ( '/' ) )
			ret.addIfNotAlreadyThere ( relative.upToFirstOccurrenceOf ( "/", false, false ) );
	}

	return ret;
}
//-----------------------------------------------------------------------------

void ZipFolder::writeFile ( const juce::String& path, const void* data, const size_t numBytes )
{
	Staged	s;
	s.path = normalizedPath ( path );
	s.data = juce::MemoryBlock ( data, numBytes );

	overlay[ lowerKey ( path ) ] = std::move ( s );
}
//-----------------------------------------------------------------------------

// A staged name that also lives in the archive must stay hidden when the
// staged version goes away
void ZipFolder::eraseOrTombstone ( const std::string& key, const juce::String& path )
{
	if ( pak.exists ( path ) )
	{
		Staged	tombstone;
		tombstone.path = normalizedPath ( path );
		tombstone.deleted = true;

		overlay[ key ] = std::move ( tombstone );
	}
	else
	{
		overlay.erase ( key );
	}
}
//-----------------------------------------------------------------------------

bool ZipFolder::rename ( const juce::String& from, const juce::String& to )
{
	const auto	fromKey = lowerKey ( from );
	const auto	toKey = lowerKey ( to );
	const auto	it = overlay.find ( fromKey );

	// The target insert comes last, so a case-only rename (same key) ends up
	// with the moved entry, not the tombstone
	if ( it != overlay.end () )
	{
		if ( it->second.deleted )
			return false;

		auto	moved = std::move ( it->second );
		moved.path = normalizedPath ( to );

		eraseOrTombstone ( fromKey, from );
		overlay[ toKey ] = std::move ( moved );
		return true;
	}

	if ( ! pak.exists ( from ) )
		return false;

	Staged	tombstone;
	tombstone.path = normalizedPath ( from );
	tombstone.deleted = true;

	Staged	target;
	target.path = normalizedPath ( to );
	target.renamedFrom = normalizedPath ( from );

	overlay[ fromKey ] = std::move ( tombstone );
	overlay[ toKey ] = std::move ( target );
	return true;
}
//-----------------------------------------------------------------------------

bool ZipFolder::remove ( const juce::String& path )
{
	const auto	key = lowerKey ( path );
	const auto	it = overlay.find ( key );

	if ( it != overlay.end () )
	{
		if ( it->second.deleted )
			return false;

		eraseOrTombstone ( key, path );
		return true;
	}

	if ( ! pak.exists ( path ) )
		return false;

	Staged	tombstone;
	tombstone.path = normalizedPath ( path );
	tombstone.deleted = true;

	overlay[ key ] = std::move ( tombstone );
	return true;
}
//-----------------------------------------------------------------------------

bool ZipFolder::removeFolder ( const juce::String& prefix )
{
	const auto	pre = normalizedPrefix ( prefix );
	auto		any = false;

	for ( const auto& relative : listFiles ( pre, true ) )
		any = remove ( pre + relative ) || any;

	return any;
}
//-----------------------------------------------------------------------------

namespace
{
	// The local header only supplies the name/extra lengths in front of the data
	bool copyRaw ( const PakFile::Entry& e, const juce::String& newPath, juce::FileInputStream& src, ZipFolder::Writer& writer )
	{
		uint8_t	lh[ 30 ];
		src.setPosition ( e.headerOffset );

		if ( src.read ( lh, 30 ) != 30 || u32 ( lh ) != 0x04034b50 )
		{
			Z_ERR ( "Bad local header for " << e.path );
			return false;
		}

		src.setPosition ( e.headerOffset + 30 + int64_t ( u16 ( lh + 26 ) ) + int64_t ( u16 ( lh + 28 ) ) );

		return writer.addRaw ( newPath, e.deflated, e.crc, e.dosDateTime, e.compressedSize, e.uncompressedSize, src );
	}
}
//-----------------------------------------------------------------------------

bool ZipFolder::commit ()
{
	if ( overlay.empty () )
		return true;

	if ( ! pak.isValid () )
		return false;

	if ( pak.getNumUnsupported () > 0 )
	{
		Z_ERR ( "Not rewriting " << getFile ().getFullPathName () << ": entries with unsupported compression would be lost" );
		return false;
	}

	juce::FileInputStream	src ( getFile () );
	if ( ! src.openedOk () )
	{
		Z_ERR ( "Cannot open " << getFile ().getFullPathName () );
		return false;
	}

	juce::TemporaryFile	temp ( getFile () );

	{
		juce::FileOutputStream	out ( temp.getFile (), 1 << 16 );
		if ( ! out.openedOk () )
		{
			Z_ERR ( "Cannot write " << temp.getFile ().getFullPathName () );
			return false;
		}

		Writer	writer ( out );

		for ( const auto& e : pak.getEntries () )
		{
			if ( overlay.contains ( lowerKey ( e.path ) ) )
				continue;

			if ( ! copyRaw ( e, e.path, src, writer ) )
				return false;
		}

		// Staged content, sorted for a deterministic rewrite
		std::vector<const Staged*>	adds;
		adds.reserve ( overlay.size () );

		for ( const auto& [ key, s ] : overlay )
			if ( ! s.deleted )
				adds.push_back ( &s );

		std::sort ( adds.begin (), adds.end (), [] ( const Staged* a, const Staged* b ) { return a->path.compareIgnoreCase ( b->path ) < 0; } );

		const auto	now = juce::Time::getCurrentTime ();

		for ( const auto* s : adds )
		{
			if ( s->renamedFrom.isNotEmpty () )
			{
				const auto*	e = pak.findEntry ( s->renamedFrom );

				if ( e == nullptr || ! copyRaw ( *e, s->path, src, writer ) )
				{
					Z_ERR ( "Lost rename source " << s->renamedFrom );
					return false;
				}
			}
			else if ( ! writer.addFile ( s->path, s->data.getData (), s->data.getSize (), now ) )
			{
				return false;
			}
		}

		if ( ! writer.finish () )
			return false;

		out.flush ();

		if ( ! out.getStatus ().wasOk () )
		{
			Z_ERR ( "Write failed: " << out.getStatus ().getErrorMessage () );
			return false;
		}
	}

	if ( ! temp.overwriteTargetFileWithTemporary () )
	{
		Z_ERR ( "Cannot replace " << getFile ().getFullPathName () );
		return false;
	}

	const auto	replaced = getFile ();

	return open ( replaced );
}
//-----------------------------------------------------------------------------
