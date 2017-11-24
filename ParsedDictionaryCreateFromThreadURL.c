/*
 *  ParsedDictionaryCreateFromThreadURL.c
 *  threadread
 *
 *  Created by mtakagi on 10/03/08.
 *  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
 *
 */

#include "ParsedDictionaryCreateFromThreadURL.h"

CFDictionaryRef ParsedDictionaryCreateFromThreadURL(CFURLRef url)
{
	CFDataRef threadData;
	CFDictionaryRef thread;
	CFMutableDictionaryRef threadDictinary;
	CFMutableDictionaryRef idDictionary;
	CFMutableArrayRef resDictionaryArray;
	CFArrayRef contents;
	CFIndex i;
	Boolean status;
	
	status = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &threadData, NULL, NULL, NULL);
	
	if (!status) {
		return NULL;
	}
	
	
// NOTE: CFPropertyListRef is a CFTypeRef!(and also return CFDictionaryRef or CFArrayRef)	
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	if (CFPropertyListCreateWithData != NULL) {
		thread = (CFDictionaryRef)CFPropertyListCreateWithData(kCFAllocatorDefault, threadData, kCFPropertyListImmutable, NULL, NULL);
	} else {
#endif
		thread = (CFDictionaryRef)CFPropertyListCreateFromXMLData(kCFAllocatorDefault, threadData, kCFPropertyListImmutable, NULL);
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1060
	}
#endif
	
	if (thread == NULL) return NULL;
	
	threadDictinary = CFDictionaryCreateMutable(kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	idDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	resDictionaryArray = CFArrayCreateMutable(kCFAllocatorDefault, 1001, &kCFTypeArrayCallBacks);
	contents = CFDictionaryGetValue(thread, CFSTR("Contents"));
	
	for (i = 0; i < CFArrayGetCount(contents); i++) {
		CFMutableDictionaryRef content = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 8, CFArrayGetValueAtIndex(contents, i));
		CFStringRef id = CFDictionaryGetValue(content, k2ChMessageID);
		
		if (i == 0) {
			CFDictionarySetValue(content, k2ChMessageThreadSubject , CFDictionaryGetValue(thread, k2ChMessageThreadSubject));
		}
		
		CFDictionaryRemoveValue(content, CFSTR("Date"));
		CFDictionaryRemoveValue(content, CFSTR("Status"));
		
		if (id != NULL && CFStringCompare(id, CFSTR(""), 0) != kCFCompareEqualTo) {
			CFNumberRef number;
			CFNumberRef newNumber;
			int n = 0;
			
			if (CFDictionaryGetValueIfPresent(idDictionary, id, (void *)&number)) {
				CFNumberGetValue(number, kCFNumberIntType, &n);
			}
			
			n += 1;
			newNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
			CFDictionarySetValue(idDictionary, id, newNumber);
			
			CFRelease(newNumber);
		}
		
		CFArrayAppendValue(resDictionaryArray, content);
		
		CFRelease(content);
	}
	
	CFDictionarySetValue(threadDictinary, RES_DICTIONARY_ARRAY, resDictionaryArray);
	CFDictionarySetValue(threadDictinary, ID_COUNT_DICTIONARY, idDictionary);
	
	CFRelease(thread);
	CFRelease(idDictionary);
	CFRelease(resDictionaryArray);
	
	return threadDictinary;
}