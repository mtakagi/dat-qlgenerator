/*
 *  ParsedDictionaryCreateFromThreadURL.c
 *  threadread
 *
 *  Created by mtakagi on 10/03/08.
 *  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
 *
 */

#include "ParsedDictionaryCreateFromThreadURL.h"

static CFDictionaryRef DictionaryCreateFromURL(CFURLRef url)
{
	CFDataRef threadData; // url から読み込んだ thread ファイルのデータ
	CFDictionaryRef thread; // 読み込んだ thread ファイルから作ったplist
	Boolean status;
	
	status = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &threadData, NULL, NULL, NULL);
	
	if (!status) {
		return NULL;
	}
	
	
	// 10.6,iOS 4.0 から CFPropertyListCreateFromXMLData は非推奨。マクロと関数が存在するかで切り分け。
	// NOTE: CFPropertyListRef is a CFTypeRef!(and also return CFDictionaryRef or CFArrayRef)	
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (CFPropertyListCreateWithData != NULL) {
		// 現在のところ CFErrorRef を無視して NULL を渡している。
		thread = (CFDictionaryRef)CFPropertyListCreateWithData(kCFAllocatorDefault, threadData, kCFPropertyListImmutable, NULL, NULL);
	} else {
#endif
		thread = (CFDictionaryRef)CFPropertyListCreateFromXMLData(kCFAllocatorDefault, threadData, kCFPropertyListImmutable, NULL);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	}
#endif
	
	
	CFRelease(threadData);
	if (thread == NULL) return NULL;
	
	return thread;
}

CFDictionaryRef ParsedDictionaryCreateFromThreadURL(CFURLRef url)
{
	CFDictionaryRef thread; // 読み込んだ thread ファイルから作ったplist
	CFMutableDictionaryRef threadDictinary; // 書き込みとIDのディクショナリ。これの値を返す。
	CFMutableDictionaryRef idDictionary; // thread ファイルに含まれる ID の情報。
	CFMutableArrayRef resDictionaryArray; // 書き込みを持つ配列。
	CFArrayRef contents; // thread ファイルの contents を入れる配列。
	CFStringRef boardName; // 板名 from BathyScaphe port
	CFIndex i;
	
	thread = DictionaryCreateFromURL(url);
	
	if (thread == NULL) return NULL;
	
	threadDictinary = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	idDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	resDictionaryArray = CFArrayCreateMutable(kCFAllocatorDefault, 1001, &kCFTypeArrayCallBacks);
	contents = CFDictionaryGetValue(thread, CFSTR("Contents"));
	boardName = CFDictionaryGetValue(thread, CFSTR("BoardName"));
    CFRetain(boardName);
	
	// content から必要な情報を取り出して resDictionaryArray に格納。
	for (i = 0; i < CFArrayGetCount(contents); i++) {
		CFMutableDictionaryRef content = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 8, CFArrayGetValueAtIndex(contents, i));
		CFStringRef id = CFDictionaryGetValue(content, k2ChMessageID);
		
		// 一番最初にスレッドのサブジェクトをいれる。
		if (i == 0) {
			CFDictionarySetValue(content, k2ChMessageThreadSubject , CFDictionaryGetValue(thread, k2ChMessageThreadSubject));
		}
		
		// 必要のない値を削除。
		CFDictionaryRemoveValue(content, CFSTR("Date"));
		CFDictionaryRemoveValue(content, CFSTR("Status"));
		
		// ID が存在する場合は ID の出現回数をチェックする。 
		if (id != NULL && CFStringCompare(id, CFSTR(""), 0) != kCFCompareEqualTo) {
			CFNumberRef number; // これまでの出現回数。
			CFNumberRef newNumber; // 現在の出現回数。 number に1を足した数。
			int n = 0; // Scalar value
			
			// number が idDictionary に存在する場合は、 n に number の数を入れる。
			if (CFDictionaryGetValueIfPresent(idDictionary, id, (void *)&number)) {
				CFNumberGetValue(number, kCFNumberIntType, &n);
			}
			
			// n をインクリメントし、 newNumber を作り idDictionary にセットする。
			n += 1;
			newNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
			CFDictionarySetValue(idDictionary, id, newNumber);
			
			CFRelease(newNumber);
		}
		
		// resDictionaryArray に追加。
		CFArrayAppendValue(resDictionaryArray, content);
		
		CFRelease(content);
	}
	
	CFDictionarySetValue(threadDictinary, RES_DICTIONARY_ARRAY, resDictionaryArray);
	CFDictionarySetValue(threadDictinary, ID_COUNT_DICTIONARY, idDictionary);
	CFDictionarySetValue(threadDictinary, CFSTR("BoardName"), boardName);
    CFRelease(boardName);
	
	// dealloc
	CFRelease(thread);
	CFRelease(idDictionary);
	CFRelease(resDictionaryArray);
	
	return threadDictinary;
}