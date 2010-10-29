/*
 *  MyCFUtilities.c
 *  dat
 *
 *  Created by mtakagi on 10/10/29.
 *  Copyright 2010 http://outofboundary.web.fc2.com/. All rights reserved.
 *
 */

#include "MyCFUtilities.h"
#include <unistd.h>
#include <asl.h>

#pragma mark Support functions

// テンポラリディレクトリのパスを返す
CFStringRef CreateTemporaryDirectoryPath()
{
	char buffer[PATH_MAX]; // 1024
	CFStringRef path;
	confstr(_CS_DARWIN_USER_TEMP_DIR, buffer, PATH_MAX); // テンポラリディレクトリのパスを取得
	path = CFStringCreateWithFileSystemRepresentation(kCFAllocatorDefault, buffer);
	
	return path;
}

// URL のファイル名の拡張子を extension で置き換える
CFStringRef CreateFileNameFromURLWithExtension(CFURLRef url, CFStringRef extension)
{
	CFURLRef deletedURL = CFURLCreateCopyDeletingPathExtension(kCFAllocatorDefault, url);
	CFStringRef number = CFURLCopyLastPathComponent(deletedURL);
	// formatOptions is unimplemented feature.
	CFStringRef fileName = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@.%@"), number, extension);
	
	CFRelease(deletedURL);
	CFRelease(number);
	
	return fileName;
}

// CFStringRef から CFDataRef を作成する
CFDataRef CreateDataFromString(CFStringRef string)
{
	CFDataRef data;
	CFIndex index = CFStringGetLength(string);
	UInt8 *bytes = (UInt8 *)malloc(sizeof(UInt8) * index);
	
	CFStringGetBytes(string, CFRangeMake(0, index), kCFStringEncodingUTF8, 0, false, bytes, index, NULL);
	data = CFDataCreate(kCFAllocatorDefault, bytes, index);
	
	free(bytes);
	
	return data;
}

// CFStringRef をテンポラリディレクトリに書き出す
void CFStringWriteToTemporary(CFStringRef string, CFStringRef fileName)
{
	CFURLRef writeURL;
	CFStringRef temporary = CreateTemporaryDirectoryPath();
	CFStringRef path = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@%@"), temporary, fileName);
	CFDataRef data = CreateDataFromString(string);
	Boolean status = false;
	SInt32 error;
	
	writeURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, path, kCFURLPOSIXPathStyle, false);
	status = CFURLWriteDataAndPropertiesToResource(writeURL, data, NULL, &error);
	
	if (status == false) {
		asl_log(NULL, NULL, ASL_LEVEL_ERR, "An error occured"); // TODO: もっと細かく
	}
	
	CFRelease(writeURL);
	CFRelease(temporary);
	CFRelease(path);
	CFRelease(data);
}

// URL のファイル名の拡張子を Uppercase にして返す
CFStringRef CopyUppercaseExtenstionString(CFURLRef url)
{
	CFStringRef extension = CFURLCopyPathExtension(url);
	CFMutableStringRef uppercase = CFStringCreateMutableCopy(kCFAllocatorDefault, CFStringGetLength(extension), extension);
	
	CFStringUppercase(uppercase, NULL);
	CFRelease(extension);
	
	return uppercase;
}