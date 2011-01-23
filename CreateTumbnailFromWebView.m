/*
 *  CreateTumbnailFromWebView.c
 *  dat

 *  Created by mtakagi on 11/01/07.
 *  Copyright 2011 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import "CreateTumbnailFromWebView.h"
#include "MyCFUtilities.h"
#include "common.h"

#ifdef DEBUG

// プロセス名が qlmanage かどうか(デバッグビルド時のみ)
// 現時点では Process Manager API を使用している (2011-1-19)
static Boolean IsQLManage()
{
	CFStringRef processName;
	ProcessSerialNumber psn;
	OSErr err = GetCurrentProcess(&psn);
	Boolean result = false;
	
	if (err == noErr) {
		CopyProcessName(&psn, &processName);
		result = CFEqual(processName, QLMANAGE);
		CFRelease(processName);
	}
	
	return result;
}

#endif

// アイコンとしての表示なのかどうかのチェック
// Finder ではアイコン表示の際には拡張子名が表示され、CoverFlow 等での表示では拡張子名は表示されないため
static Boolean ThumbnailIsIconMode(QLThumbnailRequestRef thumbnail)
{
	CFDictionaryRef options = QLThumbnailRequestCopyOptions(thumbnail);
	CFBooleanRef isIconMode;
	Boolean isValueExist = false;
	
	if (options != NULL) {
		isIconMode = NULL;
		isValueExist = CFDictionaryGetValueIfPresent(options, (const void *)kQLThumbnailOptionIconModeKey, (const void **)&isIconMode);
		CFRelease(options), options = NULL;
	}
	
	return (isValueExist && CFBooleanGetValue(isIconMode));
}

// 拡張子を描画する
static void DrawExtensionString(CGContextRef context, QLThumbnailRequestRef thumbnail)
{
	CFURLRef url; // サムネイルを作成するファイルの URL
	CFStringRef string; // サムネイルを作成するファイルの拡張子
	CFAttributedStringRef attributedString; // 描画される拡張子の Attributed string
	CTFontRef font; // 描画に使うフォント(CTLineRef で描画される横幅を調べるために使用)
	CFDictionaryRef options; // CTFont にセットする attributes
	const void *key[1]; // CTFont にセットする attributes の key
	const void *value[1]; // CTFont にセットする attributes の value
	CGFontRef cgFont; // 描画に使用するフォント(CGContext にセットするため)
	CTLineRef line; // 描画するテキストのライン
	double width; // 描画されるテキストの横幅
	CGGlyph *glyphs; // グリフ
	UniChar *text; // テキスト
	CGSize maxSize; // サムネイルの最大サイズ
	
	CGColorSpaceRef colorspace; // テキストのシャドウに使用するカラースペース
	CGColorRef color; // テキストのシャドウのカラー
	const CGFloat components[4] = { 1,1,1,1}; // テキストのシャドウのカラーに使用する値 RGBA 
	
	
	url = QLThumbnailRequestCopyURL(thumbnail);
	string = CopyUppercaseExtenstionString(url);
	maxSize = QLThumbnailRequestGetMaximumSize(thumbnail);
	font = CTFontCreateWithName(CFSTR("LucidaGrande-bold"), maxSize.height/6.5, &CGAffineTransformIdentity);
	cgFont = CTFontCopyGraphicsFont(font, NULL); // CTFontRef から CGFontRef を取得
	key[0] = (const void *)NSFontAttributeName; // TODO: CFAttributedString でのフォントの指定の仕方が若干不明
	value[0] = (const void *)font;
	options = CFDictionaryCreate(kCFAllocatorDefault, key, value, 1, 
												 &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	attributedString = CFAttributedStringCreate(kCFAllocatorDefault, string, options);
	line = CTLineCreateWithAttributedString(attributedString);
	// 描画されるテキストの横幅を取得
	width = CTLineGetTypographicBounds(line, NULL, NULL, NULL);
	
	glyphs = (CGGlyph *)calloc(CFStringGetLength(string), sizeof(CGGlyph));
	text = (UniChar *)calloc(CFStringGetLength(string), sizeof(UniChar));
	
	CFStringGetCharacters(string, CFRangeMake(0, CFStringGetLength(string)), text);
	CTFontGetGlyphsForCharacters(font, (const UniChar *)text, glyphs, CFStringGetLength(string));
	
	// 描画するテキストの設定
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	CGContextSetFont(context, cgFont);
	CGContextSetFontSize(context, maxSize.height/6.5);
//		CGContextSetCharacterSpacing(context, 0);
	CGContextSetTextDrawingMode(context, kCGTextFill);
	CGContextSetRGBFillColor(context, 0.35, 0.35, 0.35, 1);
	CGContextSetTextPosition(context, -(width/2.0), 0);

	// シャドウのカラーを作る
	colorspace = CGColorSpaceCreateDeviceRGB();
	color = CGColorCreate(colorspace, components);
	
	CGContextSaveGState(context);
//	CGContextSetBlendMode(context, kCGBlendModeSourceIn);
	CGContextSetShadowWithColor(context, CGSizeZero, 20, color); // オフセットはなしで描画
	CGContextShowGlyphs(context, glyphs, CFStringGetLength(string));
	CGContextRestoreGState(context);

	// 後始末
	if (url != NULL) CFRelease(url);
	if (string != NULL) CFRelease(string);
	if (attributedString != NULL) CFRelease(attributedString);
	if (font != NULL) CFRelease(font);
	if (options != NULL) CFRelease(options);
	if (cgFont != NULL) CGFontRelease(cgFont);
	if (line != NULL) CFRelease(line);
	if (colorspace != NULL) CGColorSpaceRelease(colorspace);
	if (color != NULL) CGColorRelease(color);
	free(glyphs);
	free(text);
}

CGContextRef CreateThumbnailFromWebView(QLThumbnailRequestRef thumbnail, CFStringRef htmlString)
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	CGContextRef context; // サムネイルを描画する CGContext
	WebView* webView;
	
	CGSize maxSize = QLThumbnailRequestGetMaximumSize(thumbnail);
	NSRect frame = NSMakeRect(0.0, 0.0, 600.0, 800.0);
	CGFloat scaleFactor = maxSize.height / 800.0;
	NSSize size = NSMakeSize(scaleFactor, scaleFactor);
	CGSize thumbnailSize = CGSizeMake(maxSize.width * (600.0 / 800.0), maxSize.height);
	webView = [[[WebView alloc] initWithFrame:frame] autorelease]; 
	
	[webView scaleUnitSquareToSize:size];
	[[[webView mainFrame] frameView] setAllowsScrolling:NO];
	[[webView mainFrame] loadHTMLString:(NSString *)htmlString baseURL:nil]; 
//	[webView stringByEvaluatingJavaScriptFromString:@"document.body.setAttribute(\"style\", \"width : 100%\")"];
	
	// WebView がロードするまで待つ
	while([webView isLoading]) { 
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true); 
	} 
	
	// TODO: position: fixed の要素があると、表示される位置がおかしくなる場合がある。
	[[[webView mainFrame] frameView] setAllowsScrolling:NO];
	context = QLThumbnailRequestCreateContext(thumbnail, thumbnailSize, false, NULL);
	
	if(context != NULL) { 
		NSGraphicsContext* nsContext = [NSGraphicsContext graphicsContextWithGraphicsPort:(void *)context 
																				  flipped:[webView isFlipped]]; 
		[webView displayRectIgnoringOpacity:[webView bounds] inContext:nsContext]; 
	} 

// DEBUG ビルド時は qlmanage を使用した際にも拡張子名を描画する。
#ifdef DEBUG
	if (IsQLManage() || ThumbnailIsIconMode(thumbnail)) {
#else // リリースビルド時はアイコンを表示する時のみ拡張子名を描画。
	if (ThumbnailIsIconMode(thumbnail)) {
#endif
		CGContextSaveGState(context);
		// 座標系をサムネイルの横幅の半分とサムネイルの高さの1/20の位置に設定
		CGContextTranslateCTM(context, thumbnailSize.width / 2.0, thumbnailSize.height/20); // hahaha
//		CGContextSetShouldSmoothFonts(context, true);
		DrawExtensionString(context, thumbnail);
		CGContextRestoreGState(context);
	}
	
	[pool release];
	
	return context;
}
