#include "ultra-shared/Helpers/PlatformHelper.h"

#if __APPLE__
#include <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

// The radius the OS put on a window's frame view, public properties only
// (the frame view's class is private, but superview/layer/cornerRadius are
// plain API). Returns 0 when the frame carries none (borderless windows,
// frame not layer-backed yet)
static CGFloat frameCornerRadius ( NSWindow* window )
{
	if ( ! window )
		return 0.0;

	auto frameView = window.contentView.superview;
	if ( frameView && frameView.layer )
		return frameView.layer.cornerRadius;

	return 0.0;
}

// The OS default, probed once from a throwaway native-frame window, for
// borderless windows, which have no frame radius of their own to read
static CGFloat defaultFrameRadius ()
{
	static const CGFloat radius = []
	{
		auto probe = [[NSWindow alloc] initWithContentRect:NSMakeRect ( 0, 0, 100, 100 )
												 styleMask:NSWindowStyleMaskTitled
												   backing:NSBackingStoreBuffered
													 defer:NO];
		const auto r = frameCornerRadius ( probe );

	#if ! __has_feature ( objc_arc )
		[probe release];
	#endif

		return r;
	} ();

	return radius;
}

void setWindowProperties ( void* windowHandle, unsigned int titleColor )
{
	auto view = (NSView*) windowHandle;

	// Preferred: whatever the OS gave this window itself, then the probed
	// OS default; hard-coded current-macOS values only as the last resort
	auto radius = frameCornerRadius ( view.window );

	if ( radius <= 0.0 )
		radius = defaultFrameRadius ();

	if ( radius <= 0.0 )
	{
		NSOperatingSystemVersion	tahoeVersion = { .majorVersion = 26, .minorVersion = 0, .patchVersion = 0 };
		bool	isTahoe = [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:tahoeVersion];

		radius = isTahoe ? 20.0f : 10.0f;
	}

	view.layer.cornerRadius = radius;

	// The title bar takes the app's color instead of the native look
	if ( NSWindow* window = view.window )
	{
		CGFloat r = ( ( titleColor >> 16 ) & 0xFF ) / 255.0;
		CGFloat g = ( ( titleColor >> 8  ) & 0xFF ) / 255.0;
		CGFloat b = ( (   titleColor     ) & 0xFF ) / 255.0;

		window.titlebarAppearsTransparent = YES;
		window.backgroundColor = [NSColor colorWithRed:r green:g blue:b alpha:1.0];
	}
}

#include <mach/mach.h>

int64_t availableMemoryBytes ()
{
	vm_size_t					pageSize = 0;
	vm_statistics64_data_t		vmstat;
	mach_msg_type_number_t		count = HOST_VM_INFO64_COUNT;

	const auto	host = mach_host_self ();
	if ( host_page_size ( host, &pageSize ) == KERN_SUCCESS
	  && host_statistics64 ( host, HOST_VM_INFO64, (host_info64_t)&vmstat, &count ) == KERN_SUCCESS )
		return int64_t ( vmstat.free_count + vmstat.inactive_count ) * int64_t ( pageSize );

	return 0;
}

void bringWindowToForeground ( void* )
{
	// LaunchServices activates freshly launched apps; nothing to reclaim
}

// Asks the user's region settings via CFNumberFormatter: exact separator and
// group sizes, independent of the BSD locale database and its naming
NumberGrouping userNumberGrouping ()
{
	NumberGrouping	g;

	auto	locale = CFLocaleCopyCurrent ();
	auto	formatter = CFNumberFormatterCreate ( kCFAllocatorDefault, locale, kCFNumberFormatterDecimalStyle );
	CFRelease ( locale );

	if ( ! formatter )
		return g;

	g.valid = true;

	auto readInt = [ formatter ] ( CFStringRef key )
	{
		int	v = 0;
		if ( auto num = (CFNumberRef) CFNumberFormatterCopyProperty ( formatter, key ) )
		{
			CFNumberGetValue ( num, kCFNumberIntType, &v );
			CFRelease ( num );
		}
		return v;
	};

	if ( const auto size = readInt ( kCFNumberFormatterGroupingSize ); size > 0 )
		g.groupSize = size;

	g.secondaryGroupSize = readInt ( kCFNumberFormatterSecondaryGroupingSize );

	if ( auto sep = (CFStringRef) CFNumberFormatterCopyProperty ( formatter, kCFNumberFormatterGroupingSeparator ) )
	{
		if ( CFStringGetLength ( sep ) > 0 )
			g.separator = wchar_t ( CFStringGetCharacterAtIndex ( sep, 0 ) );
		CFRelease ( sep );
	}

	if ( auto uses = (CFBooleanRef) CFNumberFormatterCopyProperty ( formatter, kCFNumberFormatterUseGroupingSeparator ) )
	{
		if ( ! CFBooleanGetValue ( uses ) )
			g.groupSize = 0;
		CFRelease ( uses );
	}

	CFRelease ( formatter );

	return g;
}
//-----------------------------------------------------------------------------

SignatureState verifyExecutableSignature ()
{
	// codesign already refuses to launch a modified bundle
	return SignatureState::notSigned;
}
//-----------------------------------------------------------------------------

bool firewallBlocksThisApp ()
{
	// The application firewall keys on the signature and prompts once; there
	// is no lasting cancel trap to detect
	return false;
}
//-----------------------------------------------------------------------------

#endif