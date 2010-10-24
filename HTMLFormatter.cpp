//
//  HTMLFormatter.m
//  dat
//
//  Created by mtakagi on 10/02/26.
//  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
//

#include <QuickLook/QuickLook.h>
#include "common.h"

#ifndef BUILD_FOR_BATHYSCAPHE
#include "ParsedDictionaryCreateFromDatURL.h"
#endif

#include "ParsedDictionaryCreateFromThreadURL.h"
#include "HTMLFormatter.h"

// 静的メンバ変数
const CFBundleRef HTMLFormatter::bundle = CFBundleGetBundleWithIdentifier(DAT_QLGENERATOR_BUNDLE_IDENTIFIER);
const CFURLRef HTMLFormatter::resourceFolderURL = CFBundleCopyResourcesDirectoryURL(bundle);
const CFURLRef HTMLFormatter::skinFolderURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, resourceFolderURL, CFSTR("/Skin"), true);

const CFCharacterSetRef HTMLFormatter::multiByteWhiteSpaceCharacterSet = CFCharacterSetCreateWithCharactersInRange(kCFAllocatorDefault, CFRangeMake(12288, 1));
CFMutableCharacterSetRef HTMLFormatter::aaCharacterSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);

void HTMLFormatter::init()
{
	CFCharacterSetAddCharactersInString(aaCharacterSet, CFSTR("^!~-"));
	CFCharacterSetAddCharactersInRange(aaCharacterSet, CFRangeMake(65307, 1));
	CFCharacterSetAddCharactersInRange(aaCharacterSet, CFRangeMake(65343, 1));
//	m_htmlString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	start = mach_absolute_time();
	header = NULL;
	title = NULL;
	res = NULL;
	newRes = NULL;
	m_htmlString = NULL;
	datURL = NULL;
	parsedDictionary = NULL;
	client = asl_open("HTMLFormatter", "DEBUG", ASL_OPT_NO_DELAY);
	
	asl_log(client, NULL, ASL_LEVEL_NOTICE, "Here comes the init().");
}

HTMLFormatter::HTMLFormatter() : m_isSkinChanged(true), m_isThumbnail(false)
{
	init();
	updateSkin();
}

HTMLFormatter::HTMLFormatter(const CFURLRef url) : m_isSkinChanged(true), m_isThumbnail(false) 
{	
	init();
	setURL(url);
	updateSkin();
}
	
HTMLFormatter::~HTMLFormatter()
{
	asl_log(client, NULL, ASL_LEVEL_NOTICE, "destruction!");
	CFRelease(m_htmlString);
	CFRelease(header);
	CFRelease(title);
	CFRelease(res);
	CFRelease(newRes);
	CFRelease(m_attachmentDictionary);
	CFRelease(skinFolderModificationDate);
	CFRelease(resourceFolderModificationDate);
	CFRelease(bundle);
	CFRelease(resourceFolderURL);
	CFRelease(skinFolderURL);
	CFRelease(parsedDictionary);
	CFRelease(multiByteWhiteSpaceCharacterSet);
	CFRelease(aaCharacterSet);
	asl_close(client);
	//	asl_free(msg);
}

void HTMLFormatter::sevenfourTOCid(CFMutableStringRef& tmp)
{
	CFArrayRef array;
	CFIndex i;
	CFIndex previousLength = 0;
	CFMutableStringRef script = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFCharacterSetRef characterSet = CFCharacterSetCreateWithCharactersInString(kCFAllocatorDefault, CFSTR("\") '"));
	fprintf(stderr, "sevenfour:// -> cid:\n");
	
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
		CFDataRef data;
		
		CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &data, NULL, NULL, NULL);
	
		if (data != NULL) {
			CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, extension, NULL);
			CFStringRef mime = UTTypeCopyPreferredTagWithClass(uti, kUTTagClassMIMEType);
			CFMutableDictionaryRef attachmentProps = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFMutableStringRef cid = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, CFSTR("cid:"));
			CFStringAppend(cid, fileName);
			
			CFStringFindAndReplace(tmp, sevenfourURLString, cid, sevenfourRange, 0);
			
			previousLength += CFStringGetLength(sevenfourURLString) - CFStringGetLength(cid);
			//			NSLog(@"%@ %d", cid, previousLength);
			
#ifdef DEBUG
//			asl_log(client, NULL, ASL_LEVEL_NOTICE, "Load resource \nPATH:%s UTI:%s MIME:%s\nFILENAME:%s", [path UTF8String], [uti UTF8String], [mime UTF8String], [fileName UTF8String]);
#endif		
			if (CFEqual(extension, CFSTR("css"))) {
				CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
				CFMutableStringRef recursiveTmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);
				sevenfourTOCid(recursiveTmp);
				CFStringAppendFormat(script, NULL, CFSTR("\n<style type=\"text/css\">\n%@\n</style>\n"), recursiveTmp);
#ifdef DEBUG				
//				[recursiveTmp writeToFile:[NSTemporaryDirectory() stringByAppendingString:fileName] 
//							   atomically:YES 
//								 encoding:NSUTF8StringEncoding 
//									error:nil];
#endif
				//				[releasePool release];
				//				continue;
				CFRelease(string);
				CFRelease(recursiveTmp);
			} else if (CFEqual(extension, CFSTR("js")) && !m_isThumbnail) {
				CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
				CFMutableStringRef recursiveTmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);
				sevenfourTOCid(recursiveTmp);
				CFStringAppendFormat(script, NULL, CFSTR("\n<script type=\"text/javascript\">\n%@\n</script>\n"), recursiveTmp);
#ifdef DEBUG				
//				[recursiveTmp writeToFile:[NSTemporaryDirectory() stringByAppendingString:fileName] 
//							   atomically:YES 
//								 encoding:NSUTF8StringEncoding 
//									error:nil];
#endif	
				//				[releasePool release];
				//				continue;
				CFRelease(string);
				CFRelease(recursiveTmp);
			}
			if (mime == NULL) {
				if (CFEqual(extension, CFSTR("css"))) {
					mime = CFSTR("text/css");
				} else if (CFEqual(extension, CFSTR("js"))) {
					mime = CFSTR("text/javascript");
				} else {
					mime = CFSTR("text/html");
				}
			}
			
			CFDictionarySetValue(attachmentProps, kQLPreviewPropertyAttachmentDataKey, data);
			CFDictionarySetValue(attachmentProps, kQLPreviewPropertyMIMETypeKey, mime);
			CFDictionarySetValue(m_attachmentDictionary, fileName, attachmentProps);
			
			CFRelease(uti);
			CFRelease(mime);
			CFRelease(attachmentProps);
			CFRelease(cid);
			CFRelease(data);
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
	//	NSLog(@"%@", NSStringFromRange(sevenfourSupportRange));
	if (sevenfourSupportRange.location != kCFNotFound) {
		CFStringInsert(tmp, sevenfourSupportRange.location, script);
	}
	
	
	if (array != NULL) CFRelease(array);
	CFRelease(characterSet);
	CFRelease(script);
}


void HTMLFormatter::sevenfourTOCID()
{
//	fprintf(stderr, "sevenfourTOCID()\n");
	if (m_isSkinChanged) {
//		fprintf(stderr, "skin changed\n");
		sevenfourTOCid(header);
		sevenfourTOCid(title);
	}
}

void HTMLFormatter::updateSkin()
{
//	fprintf(stderr, "updateSkin()\n");
	CFStringRef headerHTML = createStringFromURLWithFile(skinFolderURL, CFSTR("Header.html"));
	CFStringRef titleHTML = createStringFromURLWithFile(skinFolderURL, CFSTR("Title.html"));
	
	if (header != NULL) CFRelease(header);
	if (title != NULL) CFRelease(title);
	if (res != NULL) CFRelease(res);
	if (newRes != NULL) CFRelease(newRes);
	
	
	header = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, headerHTML);
	title = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, titleHTML);
	res = createStringFromURLWithFile(skinFolderURL, CFSTR("Res.html"));
	newRes = createStringFromURLWithFile(skinFolderURL, CFSTR("NewRes.html"));
	m_attachmentDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	skinFolderModificationDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, skinFolderURL, kCFURLFileLastModificationTime, NULL);
	resourceFolderModificationDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, resourceFolderURL, kCFURLFileLastModificationTime, NULL);
	sevenfourTOCID();
	
	
	CFRelease(headerHTML);
	CFRelease(titleHTML);
//	fprintf(stderr, "updateSkin() end\n");
}

void HTMLFormatter::setURL(const CFURLRef url)
{
	CFStringRef extension = CFURLCopyPathExtension(url);
	CFRetain(url);
	if (datURL != NULL) CFRelease(datURL);
	datURL = url;
	if (parsedDictionary != NULL) CFRelease(parsedDictionary);
#ifndef BUILD_FOR_BATHYSCAPHE	
	if (CFEqual(extension, CFSTR("dat"))) {
		parsedDictionary = ParsedDictionaryCreateFromDatURL(datURL);
	} else
#endif
	if (CFEqual(extension, CFSTR("thread"))) {
		parsedDictionary = ParsedDictionaryCreateFromThreadURL(datURL);
	}
	if (isSkinChanged()) {
		asl_log(client, NULL, ASL_LEVEL_NOTICE, "Skin was changed");
		updateSkin();
	}
	if (m_htmlString != NULL) CFRelease(m_htmlString);
	m_htmlString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFRelease(extension);
}

void HTMLFormatter::formatSevenfourSupport(CFMutableStringRef& tmp)
{
//	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	
//	[s appendString: @"<style type='text/css'>\n"];
//    [s appendFormat: @"body { font-family:'%@'; font-size:%.1fpx; }\n",
//	 [defaults stringForKey: @"MessageFontFamily"],
//	 [defaults floatForKey: @"MessageFontSize"]];
//    [s appendString: @"</style>\n"];
//	CFShowStr(tmp);
//	CFShow(tmp);
	if (!m_isThumbnail) {
		fprintf(stderr, "formatSevenfourSupport\n");
		CFMutableStringRef s = CFStringCreateMutable(kCFAllocatorDefault, 0);
		CFStringAppend(s, CFSTR("<script type='text/javascript'>\n"));
		CFStringAppend(s, CFSTR("sevenfour.countOfIDs = {};\n"));
		int i;
		CFIndex size;
		CFDictionaryRef idDictionary = countIDDictionary();
		size = CFDictionaryGetCount(idDictionary);
		CFTypeRef *keys = (CFTypeRef *)malloc(size * sizeof(CFTypeRef));
		CFDictionaryGetKeysAndValues(idDictionary, (const void **)keys, NULL);
		for (i = 0; i < size; i++) {
			CFStringRef key = (CFStringRef)keys[i];
			CFNumberRef count = (CFNumberRef)CFDictionaryGetValue(idDictionary, key);
			int num;
			CFNumberGetValue(count, kCFNumberIntType, &num);
			CFStringAppendFormat(s, NULL, CFSTR("sevenfour.countOfIDs['%@'] = %d;\n"), key, num);
		}
		CFStringAppend(s, CFSTR("</script>\n"));
		CFStringFindAndReplace(tmp, CFSTR("<SEVENFOUR_SUPPORT />"), s, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		free(keys);
		CFRelease(s);
//		CFShow(tmp);
	}
}

CFStringRef HTMLFormatter::formatHEADER(const CFStringRef& headerHTML)
{
//	fprintf(stderr, "formatHEADER\n");
	CFMutableStringRef tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, headerHTML);
	CFStringFindAndReplace(tmp, CFSTR("<THREADNAME />"), threadTitle(), CFRangeMake(0, CFStringGetLength(tmp)), 0);
//	CFShow(tmp);
	formatSevenfourSupport(tmp);
	return tmp;
}

CFStringRef HTMLFormatter::formatTITLE(const CFStringRef& titleHTML)
{
//	fprintf(stderr, "formatTITLE\n");
	CFMutableStringRef tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, titleHTML);
	CFStringRef urlString = CFURLCopyPath(datURL);
	CFStringFindAndReplace(tmp, CFSTR("<THREADNAME/>"), threadTitle(), CFRangeMake(0, CFStringGetLength(tmp)), 0);
	CFStringFindAndReplace(tmp, CFSTR("<THREADURL/>"), urlString, CFRangeMake(0, CFStringGetLength(tmp)), 0);
	CFRelease(urlString);
//	CFShow(tmp);
	return tmp;
}

bool HTMLFormatter::isAsciiArt(const CFStringRef& message)
{
//	fprintf(stderr, "isAsciiArt()\n");
	Boolean result;
	
	result = CFStringFindCharacterFromSet(message, multiByteWhiteSpaceCharacterSet, CFRangeMake(0, CFStringGetLength(message)), 0, NULL);
	if (result) {
		result = CFStringFindCharacterFromSet(message, aaCharacterSet, CFRangeMake(0, CFStringGetLength(message)), 0, NULL);
		
		if (result) {
//			fprintf(stderr, "isAsciiArt() end\n");
			return true;
		}
	}
//	fprintf(stderr, "isAsciiArt() end\n");
	return false;
}

void formatInternalLink(CFMutableStringRef& message)
{	
//	fprintf(stderr, "formatInternalLink\n");
	CFArrayRef array;
	CFIndex index;
	CFIndex previousLength = 0;
	
	CFStringFindAndReplace(message, CFSTR(" target=\"_blank\""), CFSTR(""), CFRangeMake(0, CFStringGetLength(message)), 0);

	array  = CFStringCreateArrayWithFindResults(kCFAllocatorDefault, message, CFSTR("<a href=\"../"), CFRangeMake(0, CFStringGetLength(message)), 0);
	if (array == NULL) return;
	
//	NSLog(@"%@", *message);
	
	for (index = 0; index < CFArrayGetCount(array); index++) {
		CFRange range = *(CFRange *)CFArrayGetValueAtIndex(array, index);
		
		CFRange result;
		CFStringFindWithOptions(message, CFSTR("\""), CFRangeMake(range.location + range.length - previousLength, CFStringGetLength(message) - range.location - range.length + previousLength), 0, &result);
		
//		NSLog(@"range.location %d range.length %d previouslength %d",range.location, range.length, previousLength);
//		NSLog(@"%@", NSStringFromRange(nsRange));
		CFStringRef substring =CFStringCreateWithSubstring(kCFAllocatorDefault, message, CFRangeMake(range.location + 9 - previousLength, result.location - range.location - 9 + previousLength));
		CFURLRef substringURL = CFURLCreateWithString(kCFAllocatorDefault, substring, NULL);
		CFStringRef lastPathComponent = CFURLCopyLastPathComponent(substringURL);
		CFStringRef number = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("#%@"), lastPathComponent);
//		NSLog(@"%@", substring);
		CFStringFindAndReplace(message, substring, number, CFRangeMake(range.location + 9 - previousLength, CFStringGetLength(substring)), 0);
		previousLength += CFStringGetLength(substring) - CFStringGetLength(number);
		
		CFRelease(substring);
		CFRelease(substringURL);
		CFRelease(lastPathComponent);
		CFRelease(number);
	}
	CFRelease(array);
}

static void formatStyleWithArray(const CFArrayRef& array, CFMutableStringRef& tmp)
{
//	fprintf(stderr, "formatstylewitharray\n");
	static CFMutableCharacterSetRef characterSet = NULL;
	CFIndex index;
	CFIndex previousLength = 0;
//	NSLog(@"%@",*tmp);
	
	if (characterSet == NULL) {
		characterSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);
		CFCharacterSetAddCharactersInString(characterSet, CFSTR(" "));
		CFCharacterSetAddCharactersInString(characterSet, CFSTR("<"));
	}
	
	for (index = 0; index < CFArrayGetCount(array); index++) {
		CFRange range = *(CFRange *)CFArrayGetValueAtIndex(array, index);
		CFRange result;
		CFStringFindCharacterFromSet(tmp, characterSet, CFRangeMake(range.location + previousLength, CFStringGetLength(tmp) - range.location - previousLength), 0, &result);

		if (result.location == kCFNotFound) {
			fprintf(stderr, "kcfnotfound");
			result.location = CFStringGetLength(tmp);
		}
		CFRange urlRange = CFRangeMake(range.location + previousLength, result.location - range.location - previousLength);
		CFStringRef url = NULL;
		CFStringRef linkedURL= NULL;
		CFStringRef substring = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, CFRangeMake(urlRange.location - 1, 1));
		CFStringRef substring2 = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, CFRangeMake(urlRange.location - 2, 2));
		
//		NSLog(@"%@", NSStringFromRange(urlRange));
		if (CFEqual(substring, CFSTR(""))) {
//			NSLog(@"Hit tp://");
			url = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, urlRange);
			linkedURL = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a href='ht%@'>%@</a>"), url, url);
		} else if (CFEqual(substring, CFSTR(" "))) {
//			NSLog(@"Hit tp://");
			url = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, urlRange);
			linkedURL = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a href='ht%@'>%@</a>"), url, url);
		} else if (urlRange.location != 1 && CFEqual(substring2, CFSTR("ht"))) {
//			NSLog(@"http");
			url = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, CFRangeMake(urlRange.location - 2, urlRange.length + 2));
			linkedURL = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a href='%@'>%@</a>"), url, url);
			urlRange.location -= 2, urlRange.length += 2;
		} else if (CFEqual(substring, CFSTR("t"))) {
//			NSLog(@"ttp");
			urlRange.location -= 1, urlRange.length += 1;
			url = CFStringCreateWithSubstring(kCFAllocatorDefault, tmp, urlRange);
			linkedURL = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a href='h%@'>%@</a>"), url, url);
		}
		if ((url != NULL) && (linkedURL != NULL)) {
			CFStringFindAndReplace(tmp, url, linkedURL, urlRange, 0);
			previousLength += CFStringGetLength(linkedURL) - CFStringGetLength(url); 
			CFRelease(url);
			CFRelease(linkedURL);
//			[*tmp replaceCharactersInRange:urlRange withString:linkedURL];
		}
		
		if (substring != NULL) CFRelease(substring);
		if (substring2 != NULL) CFRelease(substring2);
	}
//	fprintf(stderr, "formatstylewitharray end\n");
}

void formatHTTP(CFMutableStringRef& tmp)
{
//	fprintf(stderr, "formatHTTP\n");
	CFArrayRef array = CFStringCreateArrayWithFindResults(kCFAllocatorDefault, tmp, CFSTR("tp://"), CFRangeMake(0, CFStringGetLength(tmp)), 0);
	//	CFShow(array);
	
	if (array != NULL) {
		formatStyleWithArray(array, tmp);
	}
	
	if (array != NULL) CFRelease(array);
//	fprintf(stderr, "formatHTTP end\n");
}

void formatHTTPS(CFMutableStringRef& tmp)
{
//	fprintf(stderr, "formatHTTPS\n");
	CFArrayRef array = CFStringCreateArrayWithFindResults(kCFAllocatorDefault, tmp, CFSTR("tps://"), CFRangeMake(0, CFStringGetLength(tmp)), 0);
	//	CFShow(array);
	
	if (array != NULL) {
		formatStyleWithArray(array, tmp);
	}
	
	if (array != NULL) CFRelease(array);
//	fprintf(stderr, "formatHTTPS end\n");
}


void formatMessage(CFMutableStringRef& message)
{
//	fprintf(stderr, "formatMessage\n");
//	asl_log(client, NULL, ASL_LEVEL_NOTICE, "%s", [message UTF8String]);
	formatInternalLink(message);
	formatHTTP(message);
	formatHTTPS(message);
//	fprintf(stderr, "formatMessage end\n");
}

CFStringRef HTMLFormatter::formatBody()
{
	CFMutableStringRef body = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppend(body, CFSTR("<dl>"));
	CFIndex index;
	
	for (index = 0; index < CFArrayGetCount(datArray()); index++) {
//		fprintf(stderr, "message %ld\n", index);
		if (m_isThumbnail && index > 15) {
			break;
		}
		CFDictionaryRef datDictionary = (CFDictionaryRef)CFArrayGetValueAtIndex(datArray(), index);
		CFMutableStringRef tmp;
//		CFShow(datDictionary);
		CFMutableStringRef message = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageBody));
		CFStringRef plainID;
		CFStringRef plainNumber = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), index + 1);
		CFStringRef number = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a class='number' href='%d'>%d</a>"), index+1, index+1);
		CFStringRef idtag;
		CFNumberRef countOfID;
		CFStringRef countOfIDString;
		int i;
		
		
//		NSLog(@"%@", datDictionary);
//		NSLog(@"Number %d", i+1);
		
		if (isAsciiArt(message)) {
			CFStringAppendFormat(body, NULL, CFSTR("<div id='%d' class='text-art'>"), index + 1);
		} else {
			CFStringAppendFormat(body, NULL, CFSTR("<div id='%d'>"), index + 1);
		}
		
		formatMessage(message);
		
//		static BOOL isNewInserted = NO;
		
		if ((CFArrayGetCount(datArray()) - index - 1) == 50) {
			tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, newRes);
//			isNewInserted = YES;
			CFStringAppend(tmp, CFSTR("</dl><dl id='new'>"));
		} else {
			tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, res);
		}
		
		CFStringFindAndReplace(tmp, CFSTR("<PLAINNUMBER/>"), plainNumber, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		CFStringFindAndReplace(tmp, CFSTR("<NUMBER/>"), number, CFRangeMake(0, CFStringGetLength(tmp)), 0);

		if (CFEqual((CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageMail), CFSTR(""))) {
			CFStringRef mailName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a class='name' href='mailto:'><b>%@</b></a>"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageName));
			CFStringFindAndReplace(tmp, CFSTR("<MAILNAME/>"), mailName, CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFStringFindAndReplace(tmp, CFSTR("<MAIL/>"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageMail), CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFRelease(mailName);
		} else {
			CFStringRef messageName = (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageName);
			CFStringRef messageMail = (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageMail);
			CFStringRef mailName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a class='name' href='mailto:%@' title='%@'><b>%@</b></a>"), messageMail, messageMail, messageName);
			CFStringFindAndReplace(tmp, CFSTR("<MAILNAME/>"), mailName, CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFStringFindAndReplace(tmp, CFSTR("<MAIL/>"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageMail), CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFRelease(mailName);
			
		}
		
		if (!CFDictionaryGetValueIfPresent(datDictionary, (void *)k2ChMessageID, (const void **)&plainID)) {
//			NSLog(@"no id");
			idtag = CFStringCreateCopy(kCFAllocatorDefault, CFSTR(" <span class='id'>ID:?\?\?</span>"));
			countOfIDString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), 0);
			CFStringFindAndReplace(tmp, CFSTR("<USERID/>"), idtag, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		} else {
//			NSLog(@"id is %@", plainID);
			idtag = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR(" <span class='id'>ID:<a href='sevenfour://internal/popup#%@'>%@</a></span>"), plainID, plainID);
			CFStringFindAndReplace(tmp, CFSTR("<PLAINID/>"), plainID, CFRangeMake(0, CFStringGetLength(tmp)), 0);
			countOfID = (CFNumberRef)CFDictionaryGetValue(countIDDictionary(), plainID);
			CFNumberGetValue(countOfID, kCFNumberIntType, &i);
			countOfIDString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), i);
		}
		
		if (!CFDictionaryGetValueIfPresent(datDictionary, (void *)k2ChMessageTime, NULL)) {
			CFStringFindAndReplace(tmp, CFSTR("<DATE/>"), idtag, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		} else {
			CFStringRef date = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<span class='date'>%@</span>%@"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageTime),idtag);
			CFStringFindAndReplace(tmp, CFSTR("<DATE/>"), date, CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFStringFindAndReplace(tmp, CFSTR("<DATE_/>"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageTime), CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFRelease(date);
		}
		
		CFStringFindAndReplace(tmp, CFSTR("<USERID/>"), plainID, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		CFStringFindAndReplace(tmp, CFSTR("<COUNTOFID/>"), countOfIDString, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		CFStringFindAndReplace(tmp, CFSTR("<MESSAGE/>"), message, CFRangeMake(0, CFStringGetLength(tmp)), 0);
//		NSLog(@"%@", [self countID]);
//		CFShow(tmp);
		
		CFStringAppend(body, tmp);
		CFStringAppend(body, CFSTR("</div>"));
		
		CFRelease(message);
		CFRelease(plainNumber);
		CFRelease(number);
		CFRelease(idtag);
		CFRelease(tmp);
	}
	CFStringAppend(body, CFSTR("</dl>"));
	
	return body;
}

CFStringRef HTMLFormatter::htmlString()
{
	fprintf(stderr, "start htmlString()\n");
	CFStringRef headerHTML = formatHEADER(header);
	CFStringRef titleHTML = formatTITLE(title);
	CFStringRef body = formatBody();
	CFStringAppend(m_htmlString, headerHTML);
	CFStringAppend(m_htmlString, titleHTML);
	CFStringAppend(m_htmlString, body);
//	NSLog(@"%@,\n%@,\n%@", header, title, res);
	CFStringAppend(m_htmlString, CFSTR("</body></html>"));
	
	CFRelease(headerHTML);
	CFRelease(titleHTML);
	CFRelease(body);
	
	return m_htmlString;
}



