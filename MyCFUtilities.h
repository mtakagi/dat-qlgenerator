/*
 *  MyCFUtilities.h
 *  dat
 *
 *  Created by mtakagi on 10/10/29.
 *  Copyright 2010 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#ifndef MY_CF_UTILITIES_H
#define MY_CF_UTILITIES_H

#include <CoreFoundation/CoreFoundation.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* Caution: Don't use these functions. Use Cocoa equivalent.
* まあこれを使うより Cocoa フレームワークにあるものを使ったほうが無難でしょう。
* __attribute__((visibility("hidden"))) に意味があるかは不明。シンボルはたぶん .private_extern になる
*/ 

// テンポラリディレクトリのパスを返す
CFStringRef CreateTemporaryDirectoryPath() __attribute__((visibility("hidden")));
// URL のファイル名の拡張子を extension で置き換える
CFStringRef CreateFileNameFromURLWithExtension(CFURLRef url, CFStringRef extension) __attribute__((visibility("hidden")));
// CFStringRef から CFDataRef を作成する
CFDataRef CreateDataFromString(CFStringRef string) __attribute__((visibility("hidden")));
// CFStringRef をテンポラリディレクトリに書き出す
void CFStringWriteToTemporary(CFStringRef string, CFStringRef fileName) __attribute__((visibility("hidden")));
// URL のファイル名の拡張子を Uppercase にして返す
CFStringRef CopyUppercaseExtenstionString(CFURLRef url) __attribute__((visibility("hidden")));

#ifdef __cplusplus
}
#endif
	
#endif // MY_CF_UTILITIES_H