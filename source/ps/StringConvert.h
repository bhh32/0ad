typedef unsigned short jschar;
typedef unsigned short ucs2char;

struct JSString;
struct JSContext;

#include <string>

namespace StringConvert
{
	// A random collection of conversion functions:

	JSString* wstring_to_jsstring(JSContext* cx, const std::wstring& str);
	JSString* wchars_to_jsstring(JSContext* cx, const wchar_t* chars);
	JSString* wchars_to_jsstring(JSContext* cx, const wchar_t* chars, size_t len);

	void jsstring_to_wstring(JSString* str, std::wstring& result);
	void jschars_to_wstring(const jschar* chars, size_t len, std::wstring& result);
	void ucs2le_to_wstring(const char* start, const char* end, std::wstring& result);

}
