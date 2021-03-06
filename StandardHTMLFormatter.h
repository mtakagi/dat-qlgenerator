#ifndef STANDARD_HTML_FORMATTER_H
#define STANDARD_HTML_FORMATTER_H

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

#endif // STANDARD_HTML_FORMATTER_H