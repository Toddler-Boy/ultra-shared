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

void setWindowProperties ( void* windowHandle, unsigned int /*titleColor*/ )
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
#endif