//
//  HTMLFormatter.m
//  dat
//
//  Created by mtakagi on 10/02/26.
//  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
//

//#include <iostream>
#include "StandardHTMLFormatter.h"

StandardHTMLFormatter::StandardHTMLFormatter() : HTMLFormatter()
{
//	init();
	updateSkin();
//	std::cout << "Constructor" << header << std::endl;
}

StandardHTMLFormatter::StandardHTMLFormatter(const CFURLRef url) : HTMLFormatter(url)
{	
//	init();
	setURL(url);
	updateSkin();
}
	
StandardHTMLFormatter::~StandardHTMLFormatter()
{
//	CFRelease(m_htmlString);
//	CFRelease(header);
//	CFRelease(title);
//	CFRelease(m_attachmentDictionary);
//	CFRelease(skinFolderModificationDate);
//	CFRelease(resourceFolderModificationDate);
//	CFRelease(bundle);
//	CFRelease(resourceFolderURL);
//	CFRelease(skinFolderURL);
//	CFRelease(parsedDictionary);
//	CFRelease(multiByteWhiteSpaceCharacterSet);
//	CFRelease(aaCharacterSet);
//	//	asl_free(msg);
}

#pragma mark -
#pragma mark Member function
// sevenfour スキームを cid スキームに変換
void StandardHTMLFormatter::sevenfourTOCid(CFMutableStringRef& tmp)
{
	CFArrayRef array; // CFRange を格納する配列
	CFIndex i; //
	CFIndex previousLength = 0; //
	CFMutableStringRef script = CFStringCreateMutable(kCFAllocatorDefault, 0); //
	// sevenfour scheme の末端を探知するための character set ", ), , ' を探す
	CFCharacterSetRef characterSet = CFCharacterSetCreateWithCharactersInString(kCFAllocatorDefault, CFSTR("\") '"));
	
//	asl_log(client, NULL, ASL_LEVEL_DEBUG, "Convert sevenfour scheme to cid scheme\n");
	
	array = CFStringCreateArrayWithFindResults(kCFAllocatorDefault, tmp, CFSTR("sevenfour://skin"), CFRangeMake(0, CFStringGetLength(tmp)), 0);
	
	if (array == NULL) return;
	for (i = 0; i < CFArrayGetCount(array); i++) {
		CFRange range = *(CFRange *)CFArrayGetValueAtIndex(array, i);
		CFRange result;
		CFStringFindCharacterFromSet(tmp, characterSet, CFRangeMake(range.location - previousLength, CFStringGetLength(tmp) - range.location + previousLength), 0, &result);
		CFRange sevenfourRange = CFRangeMake(range.location - previousLength, result.location - range.location + previousLength);
		CFStringRef sevenfourURLString = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, sevenfourRange);
		CFURLRef sevenfourURL = CFURLCreateWithString(kCFAllocatorDefault, sevenfourURLString, NULL);
		CFStringRef pathForResource = CFStringCreateWithSubstring(kCFAllocatorDefault, sevenfourURLString, CFRangeMake(CFStringGetLength(CFSTR("sevenfour://skin")), CFStringGetLength(sevenfourURLString) - CFStringGetLength(CFSTR("sevenfour://skin"))));
		CFStringRef fileName = CFURLCopyLastPathComponent(sevenfourURL);
		CFStringRef extension = CFURLCopyPathExtension(sevenfourURL);
		CFURLRef url = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, skinFolderURL, pathForResource, false);
		
	
		if (url != NULL) {
			CFURLRef absoluteURL = CFURLCopyAbsoluteURL(url);
			CFStringRef path = CFURLGetString(absoluteURL);
			CFStringFindAndReplace(tmp, sevenfourURLString, path, sevenfourRange, 0); // sevenfour スキームを cid スキームで置き換え
//			CFShow(path);
			previousLength += CFStringGetLength(sevenfourURLString) - CFStringGetLength(path);
			
//			asl_log(client, NULL, ASL_LEVEL_DEBUG, "Load resource \nPATH:%s UTI:%s MIME:%s\nFILENAME:%s", [path UTF8String], [uti UTF8String], [mime UTF8String], [fileName UTF8String]);
			
			
			if (extension == NULL) break;
			if (CFEqual(extension, CFSTR("css")) || (CFEqual(extension, CFSTR("js")) && !m_isThumbnail)) {
				CFDataRef data;
				CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &data, NULL, NULL, NULL);
				CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
				CFMutableStringRef recursiveTmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);
				sevenfourTOCid(recursiveTmp);
				if (CFEqual(extension, CFSTR("css"))) {
					CFStringAppendFormat(script, NULL, CFSTR("\n<style type=\"text/css\">\n%@\n</style>\n"), recursiveTmp);
				} else if (CFEqual(extension, CFSTR("js"))) {
					CFStringAppendFormat(script, NULL, CFSTR("\n<style type=\"text/javascript\">\n%@\n</style>\n"), recursiveTmp);
				}
				
				CFRelease(string);
				CFRelease(recursiveTmp);
				CFRelease(data);
			} 
			
//			CFRelease(path);
//			CFShow(m_attachmentDictionary);
		}
		
		CFRelease(sevenfourURLString);
		CFRelease(sevenfourURL);
		CFRelease(pathForResource);
		CFRelease(fileName);
		if (extension != NULL) CFRelease(extension);
		CFRelease(url);
	}
	
	CFRange sevenfourSupportRange = CFStringFind(tmp, CFSTR("<SEVENFOUR_SUPPORT />"), 0);
	
	if (sevenfourSupportRange.location != kCFNotFound) {
		CFStringInsert(tmp, sevenfourSupportRange.location, script);
	}
	
	
	if (array != NULL) CFRelease(array);
	CFRelease(characterSet);
	CFRelease(script);
}
