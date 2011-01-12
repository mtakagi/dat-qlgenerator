#ifndef __STANDARD_HTML_FORMATTER__
#define __STANDARD_HTML_FORMATTER__

#include "HTMLFormatter.h"

class StandardHTMLFormatter : public HTMLFormatter {

// メンバ関数
public:	
	StandardHTMLFormatter(); // コンストラクタ
	StandardHTMLFormatter(const CFURLRef url); // コンストラクタ
	virtual ~StandardHTMLFormatter(); // デコンストラクタ
private:
	void sevenfourTOCid(CFMutableStringRef& tmp);
};

#endif // __STANDARD_HTML_FORMATTER__