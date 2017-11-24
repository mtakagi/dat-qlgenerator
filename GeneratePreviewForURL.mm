#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <QuickLook/QuickLook.h>
#import "common.h"
#include "HTMLFormatter.h"

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DEBUG	

// FIXME: 
// ほぼ意味なし	

void ShowProcessInfo(aslclient client)
{
	ProcessSerialNumber psn;
	OSStatus err;
	CFDictionaryRef dictionary;
	CFStringRef processName = NULL;
	const char *errString = NULL;
	NSProcessInfo *processInfo = [NSProcessInfo processInfo];
	
	err = GetCurrentProcess(&psn);
	if (err != noErr) {
		errString = GetMacOSStatusCommentString(err);
		asl_log(client, NULL, ASL_LEVEL_DEBUG, "ERROR:%s \ncannot get current process", errString);
		return;
	}
	
	dictionary = ProcessInformationCopyDictionary(&psn, kProcessDictionaryIncludeAllInformationMask);
	err = CopyProcessName(&psn, &processName);
	
	if (processName != NULL) {
		if (CFEqual(QLMANAGE, processName)) {
			NSLog(@"%@", [processInfo arguments]);
		}
		CFRelease(processName);
	} else {
		NSString *name = [processInfo processName];
		const char *errType = GetMacOSStatusErrorString(err);
		errString = GetMacOSStatusCommentString(err);
		asl_log(client, NULL, ASL_LEVEL_DEBUG, "ERROR:%s\nCOMMENT:%s cannot copy process name %s", errType, errString, [name UTF8String]);
	}
	if (dictionary == NULL) {
		asl_log(client, NULL, ASL_LEVEL_DEBUG, "cannot get information");
		return;
	}
	
	asl_log(client, NULL, ASL_LEVEL_DEBUG, "process info \n%s", [[(NSDictionary *)dictionary description] UTF8String]);
	
	CFRelease(dictionary);
}

#endif

// FIXME: asl関連とパフォーマンスのチェック用途のコードの整理	
	
OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
	NSLog(@"preview start");
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	static HTMLFormatter formatter;
	NSString *datHTML;
	uint64_t start = mach_absolute_time();
	uint64_t end;
	uint64_t elapsed;
	Nanoseconds elapsedNano;
	aslclient client = asl_open([(NSString *)DAT_QLGENERATOR_BUNDLE_IDENTIFIER UTF8String], NULL, ASL_OPT_STDERR);
	aslmsg msg = asl_new(ASL_TYPE_MSG);
//	asl_set(msg, ASL_KEY_SENDER, "com.fc2.web.outofboundary.qlgenerator.dat");
	asl_set(msg, ASL_KEY_FACILITY, "performance");
	
	if (QLPreviewRequestIsCancelled(preview)) goto cleanup;
	
//	if (formatter == NULL) {
//		NSLog(@"HTMLFormatter is nil");
//		formatter = HTMLFormatter(url);
//	} else {
//		NSLog(@"HTMLFormatter is cached");
		formatter.setURL(url);
//	}
	
	NSLog(@"dat -> html");
	datHTML = (NSString *)formatter.htmlString();
	
	end = mach_absolute_time();
	elapsed = end - start;
	elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);
	
	asl_log(client, NULL, ASL_LEVEL_NOTICE, "Generate preview load plugin %s %s", [(NSString *)contentTypeUTI UTF8String], [[(NSURL *)url description] UTF8String]);
	asl_log(client, msg, ASL_LEVEL_NOTICE, "\"%s\" dat -> html %fs", [(NSString *)formatter.threadTitle() UTF8String], *(uint64_t *)&elapsedNano/1000000000.0);
	
	if (datHTML != nil && !QLPreviewRequestIsCancelled(preview)) {
		NSMutableDictionary *props = [NSMutableDictionary dictionary];
		
// デバッグ用途にhtmlに変換したデータをファイルに保存する。
#ifdef DEBUG
		NSString *number = [[[(NSURL *)url path] lastPathComponent] stringByDeletingPathExtension];
		[datHTML writeToFile:[NSTemporaryDirectory() stringByAppendingFormat:@"%@.html",number] atomically:YES encoding:NSUTF8StringEncoding error:nil];
		ShowProcessInfo(client);
#endif
		[props setObject:@"text/html" forKey:(NSString *)kQLPreviewPropertyMIMETypeKey];
		[props setObject:(NSString *)formatter.threadTitle() forKey:(NSString *)kQLPreviewPropertyDisplayNameKey];
		[props setObject:@"UTF-8" forKey:(NSString *)kQLPreviewPropertyTextEncodingNameKey]; 
		[props setObject:(NSDictionary *)formatter.attachmentDictionary() forKey:(NSString *)kQLPreviewPropertyAttachmentsKey];
		
		elapsed = mach_absolute_time() - end;
		end = mach_absolute_time();
		elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);
		
		NSLog(@"Set property %fs", *(uint64_t *)&elapsedNano/1000000000.0);
		QLPreviewRequestSetDataRepresentation(preview,
											  (CFDataRef)[datHTML dataUsingEncoding:NSUTF8StringEncoding],
											  kUTTypeHTML,
											  (CFDictionaryRef)props);
		elapsed = mach_absolute_time() - end;
//		end = mach_absolute_time();
		elapsedNano = AbsoluteToNanoseconds(*(AbsoluteTime *)&elapsed);
		
		NSLog(@"QLPreviewRequestSetDataRepresentation is done %fs", *(uint64_t *)&elapsedNano/1000000000.0);
		
	}

cleanup:
	
//	[datHTML release];
	[pool release];
	asl_close(client);
	asl_free(msg);
	
    return noErr;
}

void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview)
{
    // implement only if supported
}
	
#ifdef __cplusplus
}
#endif
