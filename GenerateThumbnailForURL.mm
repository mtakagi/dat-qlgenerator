#include <QuickLook/QuickLook.h>
// #import <WebKit/WebKit.h>
#include "common.h"
//#include "HTMLFormatter.h"
#include "StandardHTMLFormatter.h"
#include "MyCFUtilities.h"
#include "CreateTumbnailFromWebView.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------------
    Generate a thumbnail for file

   This function's job is to create thumbnail for designated file as fast as possible
   ----------------------------------------------------------------------------- */
	
// TODO: 10.5でのサムネール作成
	
OSStatus GenerateThumbnailForURL(void *thisInterface, QLThumbnailRequestRef thumbnail, CFURLRef url, CFStringRef contentTypeUTI, CFDictionaryRef options, CGSize maxSize)
{
	static HTMLFormatter formatter; // ファイルを変換するクラス
	CFStringRef HTML; // 変換した HTML
	aslclient client = asl_open(BUNDLE_IDENTIFIER_CSTRING, "Thumbnail", ASL_OPT_STDERR); // stderr に出力する。

#ifdef DEBUG
	asl_set_filter(client, ASL_FILTER_MASK_UPTO(ASL_LEVEL_DEBUG)); // デバッグビルド時はロギングするレベルを DEBUG からに変更
#endif
	
	formatter.setIsThumbnail(true); // サムネイル表示に設定
	formatter.setURL(url);
	
	HTML = formatter.htmlString();
			
	if (HTML != NULL && !QLThumbnailRequestIsCancelled(thumbnail)) {		
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= 1060) && !defined(DEBUG) // サムネイルの生成に 10.6 から追加されたキーと関数を使用する
		
		if (QLThumbnailRequestSetThumbnailWithDataRepresentation != NULL) {
			CFDataRef data = CreateDataFromString(HTML);
			CFDictionaryRef attachments = CFDictionaryCreate(kCFAllocatorDefault, (const void **)kQLPreviewPropertyAttachmentsKey, 
															 (const void **)formatter.attachmentDictionary(), 1,
															 &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
			CFMutableDictionaryRef props = NULL;
			
			if (kQLThumbnailPropertyExtensionKey != NULL) {
				CFStringRef extension = CopyUppercaseExtenstionString(url);
				// CFDictionaryCreate でうまくサムネイルに拡張子名が表示できなかったので CFDictionaryCreateMutable を使用した。
				props = CFDictionaryCreateMutable(kCFAllocatorDefault,1, &kCFCopyStringDictionaryKeyCallBacks,
												  &kCFTypeDictionaryValueCallBacks);	
				CFDictionarySetValue(props, kQLThumbnailPropertyExtensionKey, extension);
				CFRelease(extension);
			}
			
			// data を使用し UTI 等を設定してサムネイルを生成。 10.6 以上でつかえる。
			// 現在のところ Reference には載っていなく header のみに記載されている。2010-10-29
			QLThumbnailRequestSetThumbnailWithDataRepresentation(thumbnail, data, kUTTypeHTML, attachments, props);
//			QLThumbnailRequestSetThumbnailWithURLRepresentation(thumbnail, url, kUTTypeHTML, (CFDictionaryRef)[NSDictionary dictionaryWithObject:[formatter attachmentDictionary] forKey:(NSString *)kQLPreviewPropertyAttachmentsKey], (CFDictionaryRef)props);
			
			// stderr には出力されるがデバッグビルド時以外はログされないはず。
			asl_log(client, NULL, ASL_LEVEL_INFO, "Use QLThumbnailRequestSetThumbnailWithDataRepresentation");
			
			CFRelease(data);
			CFRelease(attachments);
			if (props != NULL) CFRelease(props);
		} else {
#endif // 10.5 でのサムネイルの生成 未完成 DEBUG が定義されている場合はこちらでサムネイルを生成する。
			static StandardHTMLFormatter standardFormatter;
			standardFormatter.setIsThumbnail(true); // サムネイル表示に設定
			standardFormatter.setURL(url);
			
			HTML = standardFormatter.htmlString();
			asl_log(client, NULL, ASL_LEVEL_DEBUG, "CreateThumbnailFromWebView");
			CGContextRef context = CreateThumbnailFromWebView(thumbnail, HTML, maxSize);
			QLThumbnailRequestFlushContext(thumbnail, context); 
			CFRelease(context);
			// デバッグ用途にhtmlに変換したデータをファイルに保存する。
#ifdef DEBUG
//			CFStringRef fileName = CreateFileNameFromURLWithExtension(url, CFSTR("html"));
//			CFStringWriteToTemporary((CFStringRef)HTML, fileName);
//			CFRelease(fileName);
#endif
// "if (QLThumbnailRequestSetThumbnailWithDataRepresentation != NULL) {" の "} else {" を閉じるため
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= 1060) && !defined(DEBUG)
		}
#endif
	}
	
	asl_close(client);
	
    return noErr;
}

void CancelThumbnailGeneration(void* thisInterface, QLThumbnailRequestRef thumbnail)
{
    // implement only if supported
}

#ifdef __cplusplus
}
#endif
