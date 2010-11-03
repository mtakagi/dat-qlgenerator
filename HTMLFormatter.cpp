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

#pragma mark 静的メンバ変数
const CFBundleRef HTMLFormatter::bundle = CFBundleGetBundleWithIdentifier(QLGENERATOR_BUNDLE_IDENTIFIER);
const CFURLRef HTMLFormatter::resourceFolderURL = CFBundleCopyResourcesDirectoryURL(bundle);
const CFURLRef HTMLFormatter::skinFolderURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, resourceFolderURL, CFSTR("/Skin"), true);

const CFCharacterSetRef HTMLFormatter::multiByteWhiteSpaceCharacterSet = CFCharacterSetCreateWithCharactersInRange(kCFAllocatorDefault, CFRangeMake(12288, 1));
CFMutableCharacterSetRef HTMLFormatter::aaCharacterSet = CFCharacterSetCreateMutable(kCFAllocatorDefault);

#pragma mark -
#pragma mark Initializer and Contructor, Deconstructor
void HTMLFormatter::init()
{
	// AAの判定に使われる Character set を設定
	CFCharacterSetAddCharactersInString(aaCharacterSet, CFSTR("^!~-"));
	CFCharacterSetAddCharactersInRange(aaCharacterSet, CFRangeMake(65307, 1)); // '；'
	CFCharacterSetAddCharactersInRange(aaCharacterSet, CFRangeMake(65343, 1)); // '＿'

//	m_htmlString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	start = mach_absolute_time();
	// TODO: これって必要だっけ?
	header = NULL;
	title = NULL;
	res = NULL;
	newRes = NULL;
	m_htmlString = NULL;
	datURL = NULL;
	parsedDictionary = NULL;
	client = asl_open("HTMLFormatter", "DEBUG", ASL_OPT_NO_DELAY);
		
#ifdef DEBUG // デバッグビルド時に ASL_LEVEL_DEBUG レベルのログを出力をするようにする。
	asl_set_filter(client, ASL_FILTER_MASK_UPTO(ASL_LEVEL_DEBUG));
#endif

	asl_log(client, NULL, ASL_LEVEL_DEBUG, "Here comes the init().");
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
	asl_log(client, NULL, ASL_LEVEL_DEBUG, "destruction!");
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

#pragma mark -
#pragma mark Member function
// sevenfour スキームを cid スキームに変換
void HTMLFormatter::sevenfourTOCid(CFMutableStringRef& tmp)
{
	CFArrayRef array; // CFRange を格納する配列
	CFIndex i; //
	CFIndex previousLength = 0; //
	CFMutableStringRef script = CFStringCreateMutable(kCFAllocatorDefault, 0); //
	// sevenfour scheme の末端を探知するための character set ", ), , ' を探す
	CFCharacterSetRef characterSet = CFCharacterSetCreateWithCharactersInString(kCFAllocatorDefault, CFSTR("\") '"));
	
	asl_log(client, NULL, ASL_LEVEL_DEBUG, "Convert sevenfour scheme to cid scheme\n");
	
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
			// 個別対応は面倒なので UTType functions を使用して MIME type を取得する。
			// 拡張子からUTI を作成
			CFStringRef uti = UTTypeCreatePreferredIdentifierForTag(kUTTagClassFilenameExtension, extension, NULL);
			// UTI から MIME type を取得
			CFStringRef mime = UTTypeCopyPreferredTagWithClass(uti, kUTTagClassMIMEType);
			CFMutableDictionaryRef attachmentProps = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			// cid スキームの作成
			CFMutableStringRef cid = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, CFSTR("cid:"));
			CFStringAppend(cid, fileName);
			
			CFStringFindAndReplace(tmp, sevenfourURLString, cid, sevenfourRange, 0); // sevenfour スキームを cid スキームで置き換え
			
			previousLength += CFStringGetLength(sevenfourURLString) - CFStringGetLength(cid);
			
//			asl_log(client, NULL, ASL_LEVEL_DEBUG, "Load resource \nPATH:%s UTI:%s MIME:%s\nFILENAME:%s", [path UTF8String], [uti UTF8String], [mime UTF8String], [fileName UTF8String]);
		
			if (CFEqual(extension, CFSTR("css"))) {
				CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
				CFMutableStringRef recursiveTmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);
				sevenfourTOCid(recursiveTmp);
				CFStringAppendFormat(script, NULL, CFSTR("\n<style type=\"text/css\">\n%@\n</style>\n"), recursiveTmp);
#ifdef DEBUG //	書き換えたファイルのテンポラリへの保存		
				//	CFStringWriteToTemporary(recursiveTmp, fileName);		
#endif
				CFRelease(string);
				CFRelease(recursiveTmp);
			} else if (CFEqual(extension, CFSTR("js")) && !m_isThumbnail) {
				CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
				CFMutableStringRef recursiveTmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, string);
				sevenfourTOCid(recursiveTmp);
				CFStringAppendFormat(script, NULL, CFSTR("\n<script type=\"text/javascript\">\n%@\n</script>\n"), recursiveTmp);
#ifdef DEBUG //	書き換えたファイルのテンポラリへの保存			
//				CFStringWriteToTemporary(recursiveTmp, fileName);
#endif	
				CFRelease(string);
				CFRelease(recursiveTmp);
			}
			
			// MIME type が不明な場合は拡張子から判別して MIME を決める。
			if (mime == NULL) {
				if (CFEqual(extension, CFSTR("css"))) {
					mime = CFSTR("text/css");
				} else if (CFEqual(extension, CFSTR("js"))) {
					mime = CFSTR("text/javascript");
				} else {
					mime = CFSTR("text/html"); // 拡張子が css でも js でもない場合。どうして text/html にしたのか…
				}
			}
			
			assert(mime != NULL);
			
			// 
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
	
	if (sevenfourSupportRange.location != kCFNotFound) {
		CFStringInsert(tmp, sevenfourSupportRange.location, script);
	}
	
	
	if (array != NULL) CFRelease(array);
	CFRelease(characterSet);
	CFRelease(script);
}


void HTMLFormatter::sevenfourTOCID()
{
	if (m_isSkinChanged) {
		asl_log(client, NULL, ASL_LEVEL_DEBUG, "Skin was changed\n");
		sevenfourTOCid(header);
		sevenfourTOCid(title);
	}
}

// スキンの更新
void HTMLFormatter::updateSkin()
{
//	fprintf(stderr, "updateSkin()\n");
	// Header.html と Title.html は mutable copy して使用する
	CFStringRef headerHTML = createStringFromURLWithFile(skinFolderURL, CFSTR("Header.html"));
	CFStringRef titleHTML = createStringFromURLWithFile(skinFolderURL, CFSTR("Title.html"));
	
	if (header != NULL) CFRelease(header);
	if (title != NULL) CFRelease(title);
	if (res != NULL) CFRelease(res);
	if (newRes != NULL) CFRelease(newRes);
	
	// スキンのデータを読み込む
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

#pragma mark -
#pragma mark Setter
void HTMLFormatter::setURL(const CFURLRef url)
{
	CFStringRef extension = CFURLCopyPathExtension(url);
	
	// CFURLRef は外部から渡されている為 retain しておく。 retain した後 release して代入スタイル。長
	CFRetain(url);
	if (datURL != NULL) CFRelease(datURL);
	datURL = url;
	
	if (parsedDictionary != NULL) CFRelease(parsedDictionary);

#ifndef BUILD_FOR_BATHYSCAPHE // BathyScaphe 用にビルドする際は dat ファイルをサポートしない。
	if (CFEqual(extension, CFSTR("dat"))) {
		parsedDictionary = ParsedDictionaryCreateFromDatURL(datURL);
	} else
#endif
	if (CFEqual(extension, CFSTR("thread"))) {
		parsedDictionary = ParsedDictionaryCreateFromThreadURL(datURL);
	}
	
	// スキンが変更されている場合はスキンのアップデート
	if (isSkinChanged()) {
		asl_log(client, NULL, ASL_LEVEL_INFO, "Skin folder is modified");
		updateSkin();
	}
	
	if (m_htmlString != NULL) CFRelease(m_htmlString);
	m_htmlString = CFStringCreateMutable(kCFAllocatorDefault, 0);
	
	CFRelease(extension);
}

#pragma mark -
#pragma mark Format functions
// <SEVENFOUR_SUPPORT /> を処理する
void HTMLFormatter::formatSevenfourSupport(CFMutableStringRef& tmp)
{
// SevenFour のコード。 他アプリの環境設定が拾得できるか不明なためコメントアウト。
//	NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
	
//	[s appendString: @"<style type='text/css'>\n"];
//    [s appendFormat: @"body { font-family:'%@'; font-size:%.1fpx; }\n",
//	 [defaults stringForKey: @"MessageFontFamily"],
//	 [defaults floatForKey: @"MessageFontSize"]];
//    [s appendString: @"</style>\n"];

	// プレビュー時のみ
	if (!m_isThumbnail) {
//		fprintf(stderr, "formatSevenfourSupport\n");
		CFMutableStringRef s = CFStringCreateMutable(kCFAllocatorDefault, 0); // 置き換える文字列
		CFDictionaryRef idDictionary = countIDDictionary(); // ID の文字列と出現回数を保持するディクショナリ
		int i;
		CFIndex size = CFDictionaryGetCount(idDictionary);
		CFTypeRef *keys = (CFTypeRef *)malloc(size * sizeof(CFTypeRef)); // ディクショナリのキー
		
		CFStringAppend(s, CFSTR("<script type='text/javascript'>\n"));
		CFStringAppend(s, CFSTR("sevenfour.countOfIDs = {};\n"));
		
		CFDictionaryGetKeysAndValues(idDictionary, (const void **)keys, NULL);
		
		// ID のチェック等に使用する sevenfour.countOfIDs を作成
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

// <THREADNAME /> を置き換える
CFStringRef HTMLFormatter::formatHEADER(const CFStringRef& headerHTML)
{
//	fprintf(stderr, "formatHEADER\n");
	CFMutableStringRef tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, headerHTML);
	CFStringFindAndReplace(tmp, CFSTR("<THREADNAME />"), threadTitle(), CFRangeMake(0, CFStringGetLength(tmp)), 0);
//	CFShow(tmp);
	formatSevenfourSupport(tmp);
	return tmp;
}

// <THREADNAME/> と <THREADURL/> を置き換え
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

// AA の判定
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

// スレッド内部のリンクの処理
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

// ttp:// tp:// 等をリンクに変換
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
//			fprintf(stderr, "kcfnotfound");
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
		}
		
		if (substring != NULL) CFRelease(substring);
		if (substring2 != NULL) CFRelease(substring2);
	}
//	fprintf(stderr, "formatstylewitharray end\n");
}

// http をリンクにする
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

// https をリンクにする。
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

// レスの本文を整形
void formatMessage(CFMutableStringRef& message)
{
//	fprintf(stderr, "formatMessage\n");
//	asl_log(client, NULL, ASL_LEVEL_NOTICE, "%s", [message UTF8String]);
	formatInternalLink(message);
	formatHTTP(message);
	formatHTTPS(message);
//	fprintf(stderr, "formatMessage end\n");
}

// <body> を作成
CFStringRef HTMLFormatter::formatBody()
{
	CFMutableStringRef body = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppend(body, CFSTR("<dl>"));
	CFIndex index;
	
	for (index = 0; index < CFArrayGetCount(datArray()); index++) {
		// サムネイル作成時は途中で終了
		if (m_isThumbnail && index > 15) {
			break;
		}
		CFDictionaryRef datDictionary = (CFDictionaryRef)CFArrayGetValueAtIndex(datArray(), index);
		CFMutableStringRef tmp;
		CFMutableStringRef message = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageBody));
		CFStringRef plainID;
		CFStringRef plainNumber = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), index + 1);
		CFStringRef number = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<a class='number' href='%d'>%d</a>"), index+1, index+1);
		CFStringRef idtag;
		CFNumberRef countOfID;
		CFStringRef countOfIDString;
		int i;
		
		if (isAsciiArt(message)) {
			CFStringAppendFormat(body, NULL, CFSTR("<div id='%d' class='text-art'>"), index + 1);
		} else {
			CFStringAppendFormat(body, NULL, CFSTR("<div id='%d'>"), index + 1);
		}
		
		formatMessage(message);
		
//		static BOOL isNewInserted = NO;
		
		// とりあえず現在のレスから50レス前までを new にする
		if ((CFArrayGetCount(datArray()) - index - 1) == 50) {
			tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, newRes);
//			isNewInserted = YES;
			CFStringAppend(tmp, CFSTR("</dl><dl id='new'>"));
		} else {
			tmp = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, res);
		}
		
		// <PLAINNUMBER/> <NUMBER/> 置き換え。
		CFStringFindAndReplace(tmp, CFSTR("<PLAINNUMBER/>"), plainNumber, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		CFStringFindAndReplace(tmp, CFSTR("<NUMBER/>"), number, CFRangeMake(0, CFStringGetLength(tmp)), 0);

		// <MAILNAME/> <MAIL/> の置き換え。 k2chMessageMail が存在するかどうかで切り分け
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
		
		// <USERID/> <PLAINID/> の置き換え。 k2ChMessageID が存在しなければ ID は ??? になる。
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
		
		// <DATE/> <DATE_/> の置き換え。 k2ChMessageTime が存在しなければ idtag で置き換える。
		if (!CFDictionaryGetValueIfPresent(datDictionary, (void *)k2ChMessageTime, NULL)) {
			CFStringFindAndReplace(tmp, CFSTR("<DATE/>"), idtag, CFRangeMake(0, CFStringGetLength(tmp)), 0);
		} else {
			CFStringRef date = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<span class='date'>%@</span>%@"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageTime),idtag);
			CFStringFindAndReplace(tmp, CFSTR("<DATE/>"), date, CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFStringFindAndReplace(tmp, CFSTR("<DATE_/>"), (CFStringRef)CFDictionaryGetValue(datDictionary, (void *)k2ChMessageTime), CFRangeMake(0, CFStringGetLength(tmp)), 0);
			CFRelease(date);
		}
		
		CFStringRef be = NULL;
		if (CFDictionaryGetValueIfPresent(datDictionary, (void *)k2ChMessageBe, (const void **)&be)) {
			asl_log(client, NULL, ASL_LEVEL_INFO, "%s", CFStringGetCStringPtr(be, kCFStringEncodingUTF8));
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

#pragma mark -
#pragma mark Getter
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



