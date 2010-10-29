#include <QuickLook/QuickLook.h>
#include "common.h"
#include "HTMLFormatter.h"

/* -----------------------------------------------------------------------------
   Generate a preview for file

   This function's job is to create preview for designated file
   ----------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

#pragma mark -
#pragma mark Preview generator function
	
OSStatus GeneratePreviewForURL(void *thisInterface, QLPreviewRequestRef preview, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options)
{
	static HTMLFormatter formatter; // ファイルを変換する C++ のクラス
	CFStringRef HTML; // 変換した HTML
	aslclient client = asl_open(BUNDLE_IDENTIFIER_CSTRING, NULL, ASL_OPT_STDERR); // ASL の Client, stderr に出力する
// とりあえず QLGENERATOR_BUNDLE_IDENTIFIER.plist の "Logging Performance Log" キーが true の場合に経過時間をロギングする。
// 環境変数で設定したほうがいいかも。とりあえず使用停止
//	CFBooleanRef isLoggingPerformance = (CFBooleanRef)CFPreferencesCopyValue(CFSTR("Logging Preformance Log"), QLGENERATOR_BUNDLE_IDENTIFIER, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
	
	asl_log(client, NULL, ASL_LEVEL_INFO, "UTI is %s", CFStringGetCStringPtr(contentTypeUTI, kCFStringEncodingUTF8));
	
	if (QLPreviewRequestIsCancelled(preview)) goto cleanup;
	
	formatter.setURL(url); // URL をセット
	
	asl_log(client, NULL, ASL_LEVEL_INFO, "Convert to html start.");
	HTML = formatter.htmlString(); // HTML に変換
	
	if (HTML != NULL && !QLPreviewRequestIsCancelled(preview)) {
		CFMutableDictionaryRef props = CFDictionaryCreateMutable(kCFAllocatorDefault, 4, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		CFDataRef HTMLData = CreateDataFromString(HTML);
		
		// Property をセット
		CFDictionarySetValue(props, kQLPreviewPropertyMIMETypeKey , CFSTR("text/html"));
		CFDictionarySetValue(props, kQLPreviewPropertyDisplayNameKey, formatter.threadTitle());
		CFDictionarySetValue(props, kQLPreviewPropertyTextEncodingNameKey, CFSTR("UTF-8"));
		CFDictionarySetValue(props, kQLPreviewPropertyAttachmentsKey, formatter.attachmentDictionary());
		
		QLPreviewRequestSetDataRepresentation(preview, HTMLData, kUTTypeHTML, props); // Preview 開始

// デバッグ用途にhtmlに変換したデータをファイルに保存する。
#ifdef DEBUG
		CFStringRef fileName = CreateFileNameFromURLWithExtension(url, CFSTR("html"));
		CFStringWriteToTemporary((CFStringRef)HTML, fileName);
		CFRelease(fileName);
#endif
		CFRelease(props);
		CFRelease(HTMLData);
	}

//  Preference のテストに使用。
//	CFPreferencesSetValue(CFSTR("Logging Preformance Log"), (CFPropertyListRef)kCFBooleanTrue, QLGENERATOR_BUNDLE_IDENTIFIER, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
//	CFPreferencesSynchronize(QLGENERATOR_BUNDLE_IDENTIFIER, kCFPreferencesCurrentUser, kCFPreferencesAnyHost);
	
cleanup:
	
	asl_close(client);
	
    return noErr;
}

void CancelPreviewGeneration(void* thisInterface, QLPreviewRequestRef preview)
{
    // implement only if supported
}
	
#ifdef __cplusplus
}
#endif
