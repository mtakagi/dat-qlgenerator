/*
 *  CreateTumbnailFromWebView.c
 *  dat
 *
 *  Created by mtakagi on 11/01/07.
 *  Copyright 2011 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#include "CreateTumbnailFromWebView.h"

CGContextRef CreateThumbnailFromWebView(QLThumbnailRequestRef thumbnail, CFStringRef htmlString)
{
	CGSize maxSize = CGSizeMake(512.0, 512.0);
	NSRect frame = NSMakeRect(0.0, 0.0, 600.0, 800.0);
	CGFloat scaleFactor = maxSize.height / 800.0;
	NSSize size = NSMakeSize(scaleFactor, scaleFactor);
	CGSize thumbnailSize = CGSizeMake(maxSize.width * (600.0 / 800.0), maxSize.height);
	WebView* webView = [[WebView alloc] initWithFrame:frame]; 
	
	[webView scaleUnitSquareToSize:size];
	[[[webView mainFrame] frameView] setAllowsScrolling:NO];
	[[webView mainFrame] loadHTMLString:(NSString *)htmlString baseURL:nil]; 
	
	while([webView isLoading]) { 
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true); 
	} 
	
	CGContextRef context = QLThumbnailRequestCreateContext(thumbnail, thumbnailSize, false, NULL);
	
	if(context != NULL) { 
		NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:(void *)context 
																				  flipped:[webView isFlipped]]; 
		[webView displayRectIgnoringOpacity:[webView bounds] inContext:nsContext]; 
	} 
	
	[webView release];
	
	return context;
}
