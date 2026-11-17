#include "EncodingUtil.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
	// 코드 페이지 문자열 -> UTF-16
	bool ToWide(const std::string& in, unsigned int codePage, std::wstring& out) {
		out.clear();
		if (in.empty()) return true;

		int length = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, in.data(), (int)in.size(), nullptr, 0);
		if (length <= 0) return false;

		out.resize(length);
		return MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, in.data(), (int)in.size(), &out[0], length) == length;
	}

	// UTF-16 -> 코드 페이지 문자열
	bool FromWide(const std::wstring& in, unsigned int codePage, std::string& out) {
		out.clear();
		if (in.empty()) return true;

		int length = WideCharToMultiByte(codePage, 0, in.data(), (int)in.size(), nullptr, 0, nullptr, nullptr);
		if (length <= 0) return false;

		out.resize(length);
		return WideCharToMultiByte(codePage, 0, in.data(), (int)in.size(), &out[0], length, nullptr, nullptr) == length;
	}
}

bool ToUtf8(const std::string& text, unsigned int codePage, std::string& out) {
	std::wstring wide;
	if (!ToWide(text, codePage, wide)) return false;
	return FromWide(wide, CP_UTF8, out);
}

bool FromUtf8(const std::string& utf8, unsigned int codePage, std::string& out) {
	std::wstring wide;
	if (!ToWide(utf8, CP_UTF8, wide)) return false;
	return FromWide(wide, codePage, out);
}