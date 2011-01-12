/*
 *  ThreadParserCF.h
 *  threadread
 *
 *  Created by mtakagi on 10/03/08.
 *  Copyright 2010 http://outofboundary.web.fc2.com/ All rights reserved.
 *
 */

#ifndef PARSED_DICTIONARY_CREATE_FROM_THREAD_URL_H
#define PARSED_DICTIONARY_CREATE_FROM_THREAD_URL_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

// threadファイルからCFDictionaryRefを作る。
	
CFDictionaryRef ParsedDictionaryCreateFromThreadURL(CFURLRef url) __attribute__((nonnull));

#ifdef __cplusplus
}
#endif

#endif // PARSED_DICTIONARY_CREATE_FROM_THREAD_URL_H