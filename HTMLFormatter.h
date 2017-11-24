#include <CoreFoundation/CoreFoundation.h>
#include "common.h"

class HTMLFormatter {
private:
	aslclient client;
	uint64_t start;
	CFMutableStringRef header;
	CFMutableStringRef title;
	CFStringRef res;
	CFStringRef newRes;
	
protected:
	CFURLRef datURL;
	CFMutableStringRef m_htmlString;
	CFMutableDictionaryRef m_attachmentDictionary;
	CFDateRef skinFolderModificationDate;
	CFDateRef resourceFolderModificationDate;
	CFDictionaryRef parsedDictionary;
	
	bool m_isSkinChanged;
	bool m_isThumbnail;
	
	static const CFBundleRef bundle;
	static const CFURLRef skinFolderURL;
	static const CFURLRef resourceFolderURL;
	
	static CFMutableCharacterSetRef aaCharacterSet;
	static const CFCharacterSetRef multiByteWhiteSpaceCharacterSet;
	
public:	
	HTMLFormatter();
	HTMLFormatter(const CFURLRef url);
	virtual ~HTMLFormatter();
	void setURL(const CFURLRef url);
	void updateSkin();
	void setIsThumbnail();
	CFStringRef htmlString();// const { return (CFStringRef)m_htmlString; };
	CFStringRef threadTitle();
	CFDictionaryRef attachmentDictionary() const { return (CFDictionaryRef)m_attachmentDictionary; };
	bool isSkinChanged();
	void setIsThumbnail(bool isThumbnail) { m_isThumbnail = isThumbnail; };
	
private:
	void init();
	void sevenfourTOCid(CFMutableStringRef& tmp);
	void sevenfourTOCID();
	void formatSevenfourSupport(CFMutableStringRef& tmp);
	CFStringRef formatHEADER(const CFStringRef& headerHTML);
	CFStringRef formatTITLE(const CFStringRef& titleHTML);
	CFStringRef formatBody();
	CFArrayRef datArray() const { return (CFArrayRef)CFDictionaryGetValue(parsedDictionary, RES_DICTIONARY_ARRAY); };
	CFDictionaryRef countIDDictionary() const { return (CFDictionaryRef)CFDictionaryGetValue(parsedDictionary,ID_COUNT_DICTIONARY); };
	CFStringRef createStringFromURLWithFile(const CFURLRef url, const CFStringRef file);

protected:
	bool isAsciiArt(const CFStringRef& message);
};

inline bool HTMLFormatter::isSkinChanged()
{
	CFDateRef skinFolderDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, skinFolderURL, kCFURLFileLastModificationTime, NULL);
	CFDateRef resourceFolderDate = (CFDateRef)CFURLCreatePropertyFromResource(kCFAllocatorDefault, resourceFolderURL, kCFURLFileLastModificationTime, NULL);
	
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

inline CFStringRef HTMLFormatter::threadTitle()
{
	return (CFStringRef)CFDictionaryGetValue((CFDictionaryRef)CFArrayGetValueAtIndex(datArray(), 0), k2ChMessageThreadSubject);
}

inline CFStringRef HTMLFormatter::createStringFromURLWithFile(const CFURLRef url, const CFStringRef file)
{
	CFDataRef data;
	CFURLRef fileURL;
	CFStringRef string;
	
	fileURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, url, file, false);
	CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault, fileURL, &data, NULL, NULL, NULL);
	string = CFStringCreateWithBytes(kCFAllocatorDefault, CFDataGetBytePtr(data), CFDataGetLength(data), kCFStringEncodingUTF8, false);
	
	CFRelease(data);
	CFRelease(fileURL);
	
	return string;
}