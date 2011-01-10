#include <CoreFoundation/CoreFoundation.h>
#include "common.h"

class HTMLFormatter {
// メンバ変数
private:
	aslclient client; // ASL ログ出力用
	uint64_t start; // 開始した時間
	CFStringRef res; // スキンの res.html mutable copy して使用。
	CFStringRef newRes; // スキンの newRes.html mutable copy して使用。
	
protected:
	CFMutableStringRef header; // スキンの header.html
	CFMutableStringRef title; // スインの title.html

	CFURLRef datURL; // html にするファイルの URL
	CFMutableStringRef m_htmlString; // 変換した html
	CFMutableDictionaryRef m_attachmentDictionary; // 変換した html が 読み込むスキンなどのリソース
	CFDateRef skinFolderModificationDate; // スキンフォルダの変更日
	CFDateRef resourceFolderModificationDate; // リソースフォルダの変更日
	CFDictionaryRef parsedDictionary; //
	
	bool m_isSkinChanged; // スキンが変更されたかどうか
	bool m_isThumbnail; // サムネイルかどうか

// 静的メンバ変数
	static const CFBundleRef bundle; // Quicklook plugin のバンドル
	static const CFURLRef skinFolderURL; // スキンフォルダの URL
	static const CFURLRef resourceFolderURL; // リソースフォルダの URL
	
	static CFMutableCharacterSetRef aaCharacterSet; // AAの判定に使用するキャラクタセット
	static const CFCharacterSetRef multiByteWhiteSpaceCharacterSet; // 全角スペース
	
	static const CFStringRef beLinkFormat; // be のプロフィールへのリンクの雛形

// メンバ関数
public:	
	HTMLFormatter(); // コンストラクタ
	HTMLFormatter(const CFURLRef url); // コンストラクタ
	virtual ~HTMLFormatter(); // デコンストラクタ
	void setURL(const CFURLRef url); // setter 変換するファイルの URL をセット
	void updateSkin(); // スキンをアップデート
	void setIsThumbnail(); // setter サムネイルかどうかをセット ていうかなにこれ? 引数もないし…
	CFStringRef htmlString();// const { return (CFStringRef)m_htmlString; }; getter 変換された html を返す。
	CFStringRef threadTitle(); // getter スレッドのタイトルを返す。
	// getter 読み込んだリソースを返す。
	CFDictionaryRef attachmentDictionary() const { return (CFDictionaryRef)m_attachmentDictionary; };
	// getter スキンが変更されたかを返す。
	bool isSkinChanged();
	// setter サムネイルかどうかをセットする。
	void setIsThumbnail(bool isThumbnail) { m_isThumbnail = isThumbnail; };
	
private:
	virtual void sevenfourTOCid(CFMutableStringRef& tmp);
	void sevenfourTOCID();
	void formatSevenfourSupport(CFMutableStringRef& tmp);
	CFStringRef formatHEADER(const CFStringRef& headerHTML);
	CFStringRef formatTITLE(const CFStringRef& titleHTML);
	CFStringRef formatBody();
	CFArrayRef datArray() const { return (CFArrayRef)CFDictionaryGetValue(parsedDictionary, RES_DICTIONARY_ARRAY); };
	CFDictionaryRef countIDDictionary() const { return (CFDictionaryRef)CFDictionaryGetValue(parsedDictionary,ID_COUNT_DICTIONARY); };
	CFStringRef createStringFromURLWithFile(const CFURLRef url, const CFStringRef file);
	// 板名を返す
	CFStringRef boardName() const { return (CFStringRef)CFDictionaryGetValue(parsedDictionary, CFSTR("BoardName")); };

protected:
	void init(); // 初期化関数
	bool isAsciiArt(const CFStringRef& message); // AAかどうか判定する。
};

//inline
// スキンが変更されたかをチェック
inline bool HTMLFormatter::isSkinChanged()
{
	CFDateRef skinFolderDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, skinFolderURL, kCFURLFileLastModificationTime, NULL);
	CFDateRef resourceFolderDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, resourceFolderURL, kCFURLFileLastModificationTime, NULL);
	
	// メンバ変数と比較してスキンが変更されたかをチェックする。
	if (CFDateCompare(skinFolderModificationDate, skinFolderDate, NULL) == kCFCompareLessThan ||
		CFDateCompare(resourceFolderModificationDate, resourceFolderDate, NULL) == kCFCompareLessThan) {
		fprintf(stderr,"skin was changed");
		m_isSkinChanged = true;
	} else {
		m_isSkinChanged = false;
	}
	
	CFRelease(skinFolderDate);
	CFRelease(resourceFolderDate);
	
	return m_isSkinChanged;
}

// スレッドのタイトルを返す
inline CFStringRef HTMLFormatter::threadTitle()
{
	return (CFStringRef)CFDictionaryGetValue((CFDictionaryRef)CFArrayGetValueAtIndex(datArray(), 0), k2ChMessageThreadSubject);
}

// URL とファイル名からファイルの内容で CFStringRef を作る。
inline CFStringRef HTMLFormatter::createStringFromURLWithFile(const CFURLRef url, const CFStringRef file)
{
	CFDataRef data;
	CFURLRef fileURL;
	CFStringRef string;
	
	// URL とファイル名から新しく URL を作成しその URL から CFData を取得し CFStringRef に変換する。
	fileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, url, file, false);
	CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &data, NULL, NULL, NULL);
	string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
	
	CFRelease(data);
	CFRelease(fileURL);
	
	return string;
}