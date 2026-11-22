#pragma once
// 맵 선택·파티 대기 화면이 함께 쓰는 문구와 그리기 보조 함수
#include <windows.h>
#include <cstdint>
#include <string>

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