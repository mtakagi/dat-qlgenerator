/*
 *  ParsedDictionaryCreateFromDatURL.c
 *  dat
 *
 *  Created by mtakagi on 10/03/09.
 *  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
 *
 */

#include "ParsedDictionaryCreateFromDatURL.h"
#include "guess.h" // Gauche guess.h


static CFMutableDictionaryRef idDictionary;
static aslclient client;


// TODO: コメントを付ける
// JapaneseText.qlgeneratorのコードを一部改変したもの。
// CFDataRef から文字コードを判別して CFStringRef を返す。
static CFStringRef DatStringCreateFromData(CFDataRef data)
{
	const char *encoding; // guess_jp が返す文字コード。
	aslmsg msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_FACILITY, "Convert Text Encoding");
	
	encoding = guess_jp((const char *)CFDataGetBytePtr(data), CFDataGetLength(data));
	
	if (encoding != NULL) {
		// guess_jp が返した IANA 式の文字コード名を CFStringEncoding に変換する。
		CFStringRef encodingRef = CFStringCreateWithCString(kCFAllocatorDefault, encoding, kCFStringEncodingMacRoman);
		CFStringEncoding stringEncoding = CFStringConvertIANACharSetNameToEncoding(encodingRef);
		CFStringRef datString = NULL;
		
		asl_log(client, msg, ASL_LEVEL_NOTICE, "encoding is %s", encoding);
		
		// TODO: データ→テキスト変換が煩雑なので修正する。ただし CFString は指定した文字コードに invaild なものが含まれると生成できないのでそこらへんをどうにかする。
		if (stringEncoding == CFStringGetSystemEncoding()) {
			datString = CFStringCreateFromExternalRepresentation(NULL, data, CFStringGetSystemEncoding());
			asl_log(client, msg, ASL_LEVEL_DEBUG, "%s is System Encoding", encoding);
		} else if (stringEncoding == kCFStringEncodingShiftJIS) {
			datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingDOSJapanese);
		}
		if (datString == NULL) {
			datString = CFStringCreateFromExternalRepresentation(NULL, data, stringEncoding);
			
			if (datString == NULL) {
				switch (stringEncoding) {
					case kCFStringEncodingUTF8:
						datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingMacJapanese);
                        break;
                    case kCFStringEncodingMacJapanese:
                        datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingUTF8);
                        break;
					case kCFStringEncodingShiftJIS:
						datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingDOSJapanese);
                        break;
                    default:
                        datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingNonLossyASCII);
				}
			}
			
			if (datString == NULL) {
				datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingMacJapanese);
			} 
			if (datString == NULL) {
				datString = CFStringCreateFromExternalRepresentation(NULL, data, kCFStringEncodingShiftJIS_X0213);
			}
		}
		
		if (datString == NULL) {
			asl_log(client, msg, ASL_LEVEL_ERR, "Error: CFStringCreateFromExternalRepresentation() was failed");
		}
		
		asl_free(msg);
		CFRelease(encodingRef);
		
		return datString;
	}
	
	asl_log(client, msg, ASL_LEVEL_ERR, "Error: dat data -> dat string was failed");
	
	asl_free(msg);
	
	return NULL;
}

// dat ファイルの1行分を解析してディクショナリを返す。
static CFDictionaryRef ResDictionaryCreateFromLine(CFStringRef line)
{
	CFArrayRef resComponents = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, line, CFSTR("<>"));
	CFMutableDictionaryRef resDictionary;
	CFArrayRef dateAndID;
	
	if (CFArrayGetCount(resComponents) < 4) {
		CFRelease(resComponents);
		return NULL;
	}
	
	resDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 6, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    // 名前欄から </b> と <b> を削除する。
	CFMutableStringRef name = CFStringCreateMutableCopy(kCFAllocatorDefault, CFStringGetLength(CFArrayGetValueAtIndex(resComponents, 0)), CFArrayGetValueAtIndex(resComponents, 0));
    CFRange range1 = CFRangeMake(0, CFStringGetLength(name));
    CFRange range2 = CFRangeMake(0, CFStringGetLength(name) - 4);
    CFStringFindAndReplace(name, CFSTR("</b>"), CFSTR(""), range1, kCFCompareNonliteral);
    CFStringFindAndReplace(name, CFSTR("<b>"), CFSTR(""), range2, kCFCompareNonliteral);
	CFDictionarySetValue(resDictionary, k2ChMessageName, name); // 名前をセット
	CFDictionarySetValue(resDictionary, k2ChMessageMail, CFArrayGetValueAtIndex(resComponents, 1)); // メールをセット
	
    CFRelease(name);
    
	// 日付と ID 分離する。
	dateAndID = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, CFArrayGetValueAtIndex(resComponents, 2), CFSTR(" ID:"));

	if (CFArrayGetCount(dateAndID) == 2) {
		CFDictionarySetValue(resDictionary, k2ChMessageTime, CFArrayGetValueAtIndex(dateAndID, 0)); // 日付をセット
		// ID と BE を分離。
		CFArrayRef idAndBE = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, CFArrayGetValueAtIndex(dateAndID, 1), CFSTR(" BE:"));
		CFStringRef id = CFArrayGetValueAtIndex(idAndBE, 0);
		
		if (CFArrayGetCount(idAndBE) == 2) {
			// BE:~ を '-' で分割。
			CFArrayRef be = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, CFArrayGetValueAtIndex(idAndBE, 1), CFSTR("-"));
			CFDictionarySetValue(resDictionary, k2ChMessageID, CFArrayGetValueAtIndex(idAndBE, 0)); // ID をセット
			CFDictionarySetValue(resDictionary, k2ChMessageBe, be); // Be をセット
			CFRelease(be);
		} else {
			CFDictionarySetValue(resDictionary, k2ChMessageID, CFArrayGetValueAtIndex(dateAndID, 1)); // ID のみをセット
		}
		
		// ID の出現回数をチャック
		if (id != NULL && CFStringCompare(id, CFSTR(""), 0) != kCFCompareEqualTo) {
			CFNumberRef number;
			CFNumberRef newNumber;
			int n = 0;
			
			if (CFDictionaryGetValueIfPresent(idDictionary, id, (void *)&number)) {
				CFNumberGetValue(number, kCFNumberIntType, &n);
			}
			
			n++;
			newNumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &n);
			CFDictionarySetValue(idDictionary, id, newNumber);
			
			CFRelease(newNumber);
		}
		
		CFRelease(idAndBE);
		
	} else {
		CFDictionarySetValue(resDictionary, k2ChMessageTime, CFArrayGetValueAtIndex(resComponents, 2)); // 日付をセット
	}
	CFDictionarySetValue(resDictionary, k2ChMessageBody, CFArrayGetValueAtIndex(resComponents, 3)); // 本文をセット
	
	// 5つめが存在する場合はスレッドのタイトルをセット。
	if (CFStringCompare(CFArrayGetValueAtIndex(resComponents, 4), CFSTR(""), 0) != kCFCompareEqualTo) {
		CFDictionarySetValue(resDictionary, k2ChMessageThreadSubject, CFArrayGetValueAtIndex(resComponents, 4));
	}
	
	CFRelease(resComponents);
	CFRelease(dateAndID);
	
	return resDictionary;
}

CFDictionaryRef ParsedDictionaryCreateFromDatURL(CFURLRef url)
{
	CFDataRef datData; // url から作成した dat ファイルのデータ
	CFStringRef dat; // datData から作成した dat ファイルのテキスト
	CFArrayRef lines; // dat ファイルを行で分割した配列
	CFMutableArrayRef resDictionaryArray; // 
	CFMutableDictionaryRef datDictionary = NULL; // この値を返す
	Boolean status;
	client = asl_open("Parsing Dat", NULL, ASL_OPT_STDERR);
	aslmsg msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_FACILITY, "Performance");
	CFAbsoluteTime start = CFAbsoluteTimeGetCurrent();
	
	
	status = CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, url, &datData, NULL, NULL, NULL);
	
	if (!status) {
		asl_log(client, msg, ASL_LEVEL_ERR, "Getting dat data is failed");
		goto finish;
	}
	
	dat = DatStringCreateFromData(datData);
	CFRelease(datData);
	
	if (dat == NULL) {
		asl_log(client, msg, ASL_LEVEL_ERR, "Converting text encoding is failed");
		goto finish;
	}
	
	resDictionaryArray = CFArrayCreateMutable(kCFAllocatorDefault, 1001, &kCFTypeArrayCallBacks);
	datDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	idDictionary = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	
	// dat ファイルを1行づつに分割
	lines = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, dat, CFSTR("\n"));
	
	// lines からスレッドを解析した配列を作る。
	for (CFIndex i = 0; i < CFArrayGetCount(lines); i++) {
		CFDictionaryRef resDictionary = ResDictionaryCreateFromLine(CFArrayGetValueAtIndex(lines, i));
		if (resDictionary != NULL) {
			CFArrayAppendValue(resDictionaryArray, resDictionary);
		}
		if (resDictionary != NULL) CFRelease(resDictionary);
	}
	
	CFDictionarySetValue(datDictionary, RES_DICTIONARY_ARRAY, resDictionaryArray);
	CFDictionarySetValue(datDictionary, ID_COUNT_DICTIONARY, idDictionary);
	
	CFRelease(lines);
	CFRelease(resDictionaryArray);
	CFRelease(idDictionary);
	CFRelease(dat);
	
	PRINT_ELAPSE(start, client, msg, "Parsing dat done time:%fs");
	
finish:
	
	asl_close(client);
	asl_free(msg);
	
	return datDictionary;
}



