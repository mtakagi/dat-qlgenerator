#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#import <WebKit/WebKit.h>
#import "common.h"
#include "HTMLFormatter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
    Generate a thumbnail for file

   This function's job is to create thumbnail for designated file as fast as possible
   ----------------------------------------------------------------------------- */

// TODO: コードのclean up, aslとパフォーマンス用途のコードの整理、 10.5でのサムネール作成
	
OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	static HTMLFormatter formatter;
	uint64_t start = mach_absolute_time();
	uint64_t end;
	uint64_t elapsed;
	Nanoseconds elapsedNano;
	aslclient client = asl_open([(NSString *)DAT_QLGENERATOR_BUNDLE_IDENTIFIER UTF8String], "thumbnail", ASL_OPT_STDERR);
	
	
//	if (QLThumbnailRequestIsCancelled(thumbnail)) goto cleanup;
		
/*	if (formatter == nil) {
		formatter = [[HTMLFormatter alloc] initWithContentsOfURL:(NSURL *)url];
		[formatter setIsThumbnail:YES];
	} else {
		[formatter setDatURL:(NSURL *)url];
	}*/
	formatter.setIsThumbnail(true);
	formatter.setURL(url);
	
	NSString *datHTML = (NSString *)formatter.htmlString();
	
	end = mach_absolute_time();
	elapsed = end - start;
	elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);
	
//	asl_log(client, NULL, ASL_LEVEL_NOTICE, "\"%s\" dat -> thumbnail %fs", [[formatter threadTitle] UTF8String], *(uint64_t *)&elapsedNano/10000000000.0);
	
	if (datHTML != nil && !QLThumbnailRequestIsCancelled(thumbnail)) {		
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		NSDictionary *props = nil;
		if (kQLThumbnailPropertyExtensionKey != NULL) {
			NSString *extension = [[(NSString *)CFURLCopyPathExtension(url) autorelease] uppercaseString];
			props = [NSDictionary dictionaryWithObject:extension forKey:(NSString *)kQLThumbnailPropertyExtensionKey];
			NSLog(@"Set kQLThumbnailPropertyExtensionKey(is a \"%@\")", (NSString *)kQLThumbnailPropertyExtensionKey);
		}
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		if (QLThumbnailRequestSetThumbnailWithDataRepresentation != NULL) {
			QLThumbnailRequestSetThumbnailWithDataRepresentation(thumbnail, (CFDataRef)[datHTML dataUsingEncoding:NSUTF8StringEncoding], kUTTypeHTML, (CFDictionaryRef)[NSDictionary dictionaryWithObject:(NSDictionary *)formatter.attachmentDictionary() forKey:(NSString *)kQLPreviewPropertyAttachmentsKey], (CFDictionaryRef)props);
//			QLThumbnailRequestSetThumbnailWithURLRepresentation(thumbnail, url, kUTTypeHTML, (CFDictionaryRef)[NSDictionary dictionaryWithObject:[formatter attachmentDictionary] forKey:(NSString *)kQLPreviewPropertyAttachmentsKey], (CFDictionaryRef)props);
			end = mach_absolute_time();
			elapsed = end - start;
			elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);
			asl_log(client, NULL, ASL_LEVEL_NOTICE, "datarepresentaion %fs", *(uint64_t *)&elapsedNano/10000000000.0);
			NSLog(@"Use QLThumbnailRequestSetThumbnailWithDataRepresentation");
		} else {
#endif
#if 0
		NSRect frame = NSMakeRect(0.0, 0.0, 600.0, 800.0);
		CGFloat scaleFactor = maxSize.height / 800.0;
		NSSize size = NSMakeSize(scaleFactor, scaleFactor);
		CGSize thumbnailSize = CGSizeMake(maxSize.width * (600.0 / 800.0), maxSize.height);
		WebView* webView = [[[WebView alloc] initWithFrame:frame] autorelease]; 
		
		[webView scaleUnitSquareToSize:size];
		[[[webView mainFrame] frameView] setAllowsScrolling:NO];
		[[webView mainFrame] loadHTMLString:datHTML baseURL:nil]; 
		
		while([webView isLoading]) { 
			CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true); 
		} 
		
		CGContextRef context = QLThumbnailRequestCreateContext(thumbnail, thumbnailSize, false, 
															   (CFDictionaryRef)props); 
		if(context != NULL) { 
			NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:(void *)context 
																					  flipped:[webView isFlipped]]; 
			[webView displayRectIgnoringOpacity:[webView bounds] inContext:nsContext]; 
			QLThumbnailRequestFlushContext(thumbnail, context); 
			CFRelease(context); 
		} 
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
		}
#endif
	}
	
//cleanup:
	[pool release];
	
    return noErr;
}

void CancelThumbnailGeneration(void* thisInterface, QLThumbnailRequestRef thumbnail)
{
    // implement only if supported
}

#ifdef __cplusplus
}
#endif
