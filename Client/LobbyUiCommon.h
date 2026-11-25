#pragma once
// 맵 선택·파티 대기 화면이 함께 쓰는 문구와 그리기 보조 함수
#include <windows.h>
#include <cstdint>
#include <string>

#pragma comment(lib, "msimg32.lib")

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 940;

// 서버의 거절 응답(REQUEST_REJECT)이 담은 errorCode를 화면 문구로 바꾼다.
// 숫자는 서버의 PartyError 값과 같아야 한다. (1 잘못된 스테이지, 2 공략 중, 3 정원 초과, 4 이미 참가 중, 5 파티 없음, 6 방장 아님)
inline const wchar_t* PartyErrorText(uint16_t code)
{
    switch (code) {
    case 1:  return L"존재하지 않는 스테이지입니다.";
    case 2:  return L"이미 공략 중인 스테이지입니다.";
    case 3:  return L"파티 정원이 가득 찼습니다.";
    case 4:  return L"이미 파티에 참가 중입니다.";
    case 5:  return L"참가 중인 파티가 없습니다.";
    case 6:  return L"방장만 시작할 수 있습니다.";
    default: return L"서버가 요청을 거절했습니다.";
    }
}


inline std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0);
    if (length <= 0) return L"?";
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), (int)text.size(), &wide[0], length);
    return wide;
}

// UTF-16 문자열을 UTF-8로 바꿈, 서버와 주고받는 문자열이 UTF-8이므로 소스의 한글을 서버 규격에 맞춰 만들 때 사용
// 좁은 문자열 리터럴은 컴파일러가 시스템 코드 페이지로 저장하므로 한글은 와이드 리터럴로 쓰고 이 함수로 바꿈
inline std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty()) return std::string();
    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
    if (length <= 0) return std::string();
    std::string utf8(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), &utf8[0], length, nullptr, nullptr);
    return utf8;
}

inline void DrawLabel(HDC hdc, const RECT& area, const std::wstring& text, COLORREF color, int fontHeight, UINT format, bool bold = false)
{
    HFONT font = CreateFontW(fontHeight, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    RECT rect = area;
    DrawTextW(hdc, text.c_str(), -1, &rect, format);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

inline void DrawBox(HDC hdc, const RECT& area, COLORREF fill, COLORREF border)
{
    HBRUSH fillBrush = CreateSolidBrush(fill);
    FillRect(hdc, &area, fillBrush);
    DeleteObject(fillBrush);

    HBRUSH borderBrush = CreateSolidBrush(border);
    FrameRect(hdc, &area, borderBrush);
    DeleteObject(borderBrush);
}

// 마젠타를 투명색으로 쓰는 BMP 리소스 도우미
constexpr COLORREF kTransparentColor = RGB(255, 0, 255);

inline HBITMAP LoadResourceBitmap(int resourceId)
{
    return (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}

// 마젠타를 투명색으로 두고 이미지를 화면 좌표에 그림
inline void DrawTransparentBitmap(HDC hdc, HBITMAP bitmap, int x, int y)
{
    if (bitmap == nullptr) return;

    BITMAP info;
    GetObject(bitmap, sizeof(info), &info);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
    TransparentBlt(hdc, x, y, info.bmWidth, info.bmHeight, memDC, 0, 0, info.bmWidth, info.bmHeight, kTransparentColor);
    SelectObject(memDC, oldBitmap);
    DeleteDC(memDC);
}
