/*
 *  CreateTumbnailFromWebView.h
 *  dat
 *
 *  Created by mtakagi on 11/01/07.
 *  Copyright 2011 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#include <QuickLook/QuickLook.h>

#ifdef __cplusplus
extern "C" {
#endif

CGContextRef CreateThumbnailFromWebView(QLThumbnailRequestRef thumbnail, CFStringRef htmlString);

#ifdef __cplusplus
}
#endif