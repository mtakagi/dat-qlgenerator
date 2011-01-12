/*
 *  CreateTumbnailFromWebView.h
 *  dat
 *
 *  Created by mtakagi on 11/01/07.
 *  Copyright 2011 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#ifndef CREATE_THUMBNAIL_FROM_WEBVIEW_H
#define CREATE_THUMBNAIL_FROM_WEBVIEW_H

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include <QuickLook/QuickLook.h>

#ifdef __cplusplus
extern "C" {
#endif

// Based on Quick Look Mailing list and other open source project.
	
CGContextRef CreateThumbnailFromWebView(QLThumbnailRequestRef thumbnail, CFStringRef htmlString, CGSize maxSize);

#ifdef __cplusplus
}
#endif

#endif // CREATE_THUMBNAIL_FROM_WEBVIEW_H