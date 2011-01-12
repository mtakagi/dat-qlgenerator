/*
 *  CreateTumbnailFromWebView.c
 *  dat
 *
 *  Created by mtakagi on 11/01/07.
 *  Copyright 2011 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#import "CreateTumbnailFromWebView.h"

CGContextRef CreateThumbnailFromWebView(QLThumbnailRequestRef thumbnail, CFStringRef htmlString)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CGSize maxSize = QLThumbnailRequestGetMaximumSize(thumbnail);
	NSRect frame = NSMakeRect(0.0, 0.0, 600.0, 800.0);
	CGFloat scaleFactor = maxSize.height / 800.0;
	NSSize size = NSMakeSize(scaleFactor, scaleFactor);
	CGSize thumbnailSize = CGSizeMake(maxSize.width * (600.0 / 800.0), maxSize.height);
	WebView* webView = [[[WebView alloc] initWithFrame:frame] autorelease]; 
	
	[webView scaleUnitSquareToSize:size];
	[[[webView mainFrame] frameView] setAllowsScrolling:NO];
	[[webView mainFrame] loadHTMLString:(NSString *)htmlString baseURL:nil]; 
//	[webView stringByEvaluatingJavaScriptFromString:@"document.body.setAttribute(\"style\", \"width : 100%\")"];
	
	while([webView isLoading]) { 
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true); 
	} 
	
	[[[webView mainFrame] frameView] setAllowsScrolling:NO];
//	[[[[webView mainFrame] frameView] documentView] setFrame:[[[webView mainFrame] frameView] frame]];
//	[[[[webView mainFrame] frameView] documentView] layout];
	CGContextRef context = QLThumbnailRequestCreateContext(thumbnail, thumbnailSize, false, NULL);
//	NSLog(@"WebFrameView = %@\nDocumentView = %@", NSStringFromRect([[[webView mainFrame] frameView] frame])
//		  , NSStringFromRect([[[[webView mainFrame] frameView] documentView] frame]));
//	NSLog(@"%@", [webView stringByEvaluatingJavaScriptFromString:@"document.body.style.cssText"]);
	
	if(context != NULL) { 
		NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:(void *)context 
																				  flipped:[webView isFlipped]]; 
		[webView displayRectIgnoringOpacity:[webView bounds] inContext:nsContext]; 
	} 
	
	[pool release];
	
	return context;
}
