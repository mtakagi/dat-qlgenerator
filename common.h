/*
 *  common.h
 *  dat
 *
 *  Created by mtakagi on 10/03/09.
 *  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <asl.h>
#include <mach/mach_time.h>

#ifdef __OBJC__
#include <Foundation/Foundation.h>
#endif


// SevenFourからもってきた。.thread互換用途に中身を入れ替えてある。
#define k2ChMessageName CFSTR("Name")
#define k2ChMessageMail CFSTR("Mail")
#define k2ChMessageBody CFSTR("Message")
#define k2ChMessageTime CFSTR("DateRepresentation")
#define k2ChMessageID   CFSTR("ID")
#define k2ChMessageBe   CFSTR("BeProfileLink")
#define k2ChMessageThreadSubject CFSTR("Title")
#define k2chMessageBoardName CFSTR("BoardName") // from BathyScaphe port

#define ID_COUNT_DICTIONARY CFSTR("IDCountDictionary")
#define RES_DICTIONARY_ARRAY CFSTR("ResDictionaryArray")

// CFBundleIdentifier をビルド時のフラグによって決める
#ifdef BUILD_FOR_BATHYSCAPHE // BathyScaphe 用
#define BUNDLE_IDENTIFIER_CSTRING "jp.tsawada2.BathyScaphe.qlgenerator"
#else // 通常時
#define BUNDLE_IDENTIFIER_CSTRING "com.fc2.web.outofboundary.qlgenerator.dat"
#endif

#define QLGENERATOR_BUNDLE_IDENTIFIER CFSTR(BUNDLE_IDENTIFIER_CSTRING)


// テスト用途
#define PRINT_ELAPSE(start, client, msg, format) {\
   uint64_t end;\
   uint64_t elapsed;\
   Nanoseconds elapsedNano;\
   end = mach_absolute_time();\
   elapsed = end - start;\
   elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);\
   asl_log(client, msg, ASL_LEVEL_NOTICE, format, *(uint64_t *)&elapsedNano/1000000000.0);\
};

#define QLMANAGE CFSTR("qlmanage")
#define QUICKLOOKD CFSTR("quicklookd")

#endif // __COMMON_H__