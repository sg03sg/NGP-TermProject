#include "ChatUI.h"
#include "GameWindow.h"
#include "LobbyUiCommon.h"
#include "PartyUI.h"
#include "../resource.h"
#include "../RendererGlobals.h"
#include <cstdio>

namespace {
    // 채팅 아이콘과 채팅창 이미지 크기
    constexpr int kIconWidth = 64;
    constexpr int kIconHeight = 60;
    constexpr int kIconGap = 8;           // 파티 창 아래쪽과 아이콘 사이
    constexpr int kWindowWidth = 360;
    constexpr int kWindowHeight = 480;
    constexpr int kWindowGap = 10;        // 파티 창과 채팅창 사이

    // 전투 화면의 채팅창 배치
    constexpr int kBattleWindowLeft = 10;   // 화면 왼쪽 끝에서 띄움
    constexpr int kBattleWindowGap = 60;    // 왼쪽 위의 마나 표시 아래로 띄우는 간격

    // 채팅창 이미지 안쪽의 입력 영역
    constexpr int kInputLeft = 20;
    constexpr int kInputTop = 414;
    constexpr int kInputWidth = 320;
    constexpr int kInputHeight = 44;
    constexpr int kInputPadding = 8;

    constexpr size_t kMaxChatBytes = 100;   // 계획서의 채팅 본문 제한, 서버로 보내는 UTF-8 바이트 수 기준

    bool IsHighSurrogate(wchar_t ch) { return ch >= 0xD800 && ch <= 0xDBFF; }
    bool IsLowSurrogate(wchar_t ch) { return ch >= 0xDC00 && ch <= 0xDFFF; }

    // UTF-16 문자열을 UTF-8로 바꿨을 때의 바이트 수
    size_t Utf8Length(const std::wstring& text)
    {
        size_t bytes = 0;
        for (size_t i = 0; i < text.size(); ++i) {
            const wchar_t ch = text[i];
            if (ch < 0x80) bytes += 1;
            else if (ch < 0x800) bytes += 2;
            else if (IsHighSurrogate(ch) && i + 1 < text.size() && IsLowSurrogate(text[i + 1])) { bytes += 4; ++i; }   // 이모지처럼 두 글자가 한 문자
            else bytes += 3;                                                                                         // 한글, 한자 등
        }
        return bytes;
    }
    std::string WideToUtf8(const std::wstring& text)
    {
        if (text.empty()) return std::string();
        const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return std::string();
        std::string utf8(length, '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.data(), (int)text.size(), &utf8[0], length, nullptr, nullptr);
        return utf8;
    }

    // 콘솔에 보여주기 위해 시스템 코드 페이지 문자열로 바꿈
    std::string WideToConsole(const std::wstring& text)
    {
        if (text.empty()) return std::string();
        const int length = WideCharToMultiByte(CP_ACP, 0, text.data(), (int)text.size(), nullptr, 0, nullptr, nullptr);
        if (length <= 0) return std::string();
        std::string ansi(length, '\0');
        WideCharToMultiByte(CP_ACP, 0, text.data(), (int)text.size(), &ansi[0], length, nullptr, nullptr);
        return ansi;
    }

    // 입력 중인 문장을 한 줄로 표시, 앞쪽 글자 + 조합 중인 글자 + 뒤쪽 글자 순서로 이어 그림
    // 조합 중인 글자에는 밑줄, 커서 위치에는 세로선, 커서가 영역 밖이면 보이도록 글자를 왼쪽으로 밀어서 표시
    void DrawDraft(HDC hdc, const RECT& area, const std::wstring& before, const std::wstring& composing, const std::wstring& after)
    {
        HFONT font = CreateFontW(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕");
        HFONT oldFont = (HFONT)SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));

        TEXTMETRICW metric;
        GetTextMetricsW(hdc, &metric);

        auto widthOf = [hdc](const std::wstring& text) {
            SIZE size = { 0, 0 };
            if (!text.empty()) GetTextExtentPoint32W(hdc, text.c_str(), (int)text.size(), &size);
            return (int)size.cx;
        };

        const int beforeWidth = widthOf(before);
        const int composingWidth = widthOf(composing);
        const int caretOffset = beforeWidth + composingWidth;

        const int visibleWidth = (area.right - area.left) - 2;
        const int scroll = caretOffset > visibleWidth ? caretOffset - visibleWidth : 0;
        const int left = area.left - scroll;
        const int top = area.top + ((area.bottom - area.top) - metric.tmHeight) / 2;

        const int saved = SaveDC(hdc);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);

        TextOutW(hdc, left, top, before.c_str(), (int)before.size());
        TextOutW(hdc, left + beforeWidth, top, composing.c_str(), (int)composing.size());
        TextOutW(hdc, left + caretOffset, top, after.c_str(), (int)after.size());

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        SelectObject(hdc, pen);

        if (!composing.empty()) {
            const int underline = top + metric.tmHeight - 1;
            MoveToEx(hdc, left + beforeWidth, underline, nullptr);
            LineTo(hdc, left + caretOffset, underline);
        }

        MoveToEx(hdc, left + caretOffset, top, nullptr);
        LineTo(hdc, left + caretOffset, top + metric.tmHeight);

        RestoreDC(hdc, saved);
        DeleteObject(pen);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }
}

ChatUI::ChatUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId) : state_(state), send_(std::move(send)), nextRequestId_(std::move(nextRequestId))
{

}

ChatUI::~ChatUI()
{
    if (iconBitmap_) DeleteObject(iconBitmap_);
    if (windowBitmap_) DeleteObject(windowBitmap_);
    if (bubbleBitmap_) DeleteObject(bubbleBitmap_);
}

void ChatUI::LoadBitmaps()
{
    if (iconBitmap_ == nullptr)   iconBitmap_ = LoadResourceBitmap(IDB_CHAT_ICON);
    if (windowBitmap_ == nullptr) windowBitmap_ = LoadResourceBitmap(IDB_CHAT_WINDOW);
    if (bubbleBitmap_ == nullptr) bubbleBitmap_ = LoadResourceBitmap(IDB_CHAT_BUBBLE);
}

RECT ChatUI::ToggleButtonRect() const
{
    if (state_.Screen() == ScreenState::Battle) {
        // 전투 화면은 하단 병종 UI의 오른쪽에 아래쪽을 맞춰 붙임
        const int left = uis[0].x + uis[0].length + kIconGap;
        const int bottom = uis[0].y + uis[0].height;
        return RECT{ left, bottom - kIconHeight, left + kIconWidth, bottom };
    }

    // 파티 창의 오른쪽 아래 모서리 바로 밑, 채팅창이 열려도 가려지지 않는 자리
    const RECT party = PartyUI::PanelRect();
    return RECT{ party.right - kIconWidth, party.bottom + kIconGap, party.right, party.bottom + kIconGap + kIconHeight };
}

RECT ChatUI::WindowRect() const
{
    if (state_.Screen() == ScreenState::Battle) {
        // 왼쪽 위의 마나 표시와 하단 병종 UI를 가리지 않도록 그 사이의 왼쪽에 배치
        const int top = uis[2].y + uis[2].height + kBattleWindowGap;
        return RECT{ kBattleWindowLeft, top, kBattleWindowLeft + kWindowWidth, top + kWindowHeight };
    }

    // 파티 창을 가리지 않도록 파티 창의 오른쪽에 같은 높이로 배치
    const RECT party = PartyUI::PanelRect();
    return RECT{ party.right + kWindowGap, party.top, party.right + kWindowGap + kWindowWidth, party.top + kWindowHeight };
}

void ChatUI::Toggle()
{
    open_ = !open_;
    composing_.clear();
    pendingHigh_ = 0;
    GameWindow_Invalidate();
}

void ChatUI::InsertAtCaret(const std::wstring& text)
{
    // 글자 수가 아니라 서버로 보낼 UTF-8 바이트 수가 100을 넘지 않을 만큼만 문자 단위로 넣음
    size_t used = Utf8Length(draft_);
    std::wstring part;

    for (size_t i = 0; i < text.size(); ) {
        size_t unit = 1;   // 이모지처럼 두 글자가 한 문자면 함께 넣거나 함께 버림
        if (IsHighSurrogate(text[i]) && i + 1 < text.size() && IsLowSurrogate(text[i + 1])) unit = 2;

        const std::wstring one = text.substr(i, unit);
        const size_t cost = Utf8Length(one);
        if (used + cost > kMaxChatBytes) break;

        part += one;
        used += cost;
        i += unit;
    }

    draft_.insert(caret_, part);
    caret_ += part.size();
}

void ChatUI::OnChar(wchar_t ch)
{
    if (!open_) return;

    if (ch == L'\b') {
        if (caret_ == 0) return;

        // 이모지처럼 두 글자 단위로 저장된 문자는 함께 지움
        size_t count = 1;
        if (caret_ >= 2 && IsLowSurrogate(draft_[caret_ - 1]) && IsHighSurrogate(draft_[caret_ - 2])) count = 2;

        draft_.erase(caret_ - count, count);
        caret_ -= count;
    }
    else if (IsHighSurrogate(ch)) {
        pendingHigh_ = ch;   // 이모지는 앞 글자가 먼저 오므로 뒤 글자가 올 때까지 기다림
        return;
    }
    else if (IsLowSurrogate(ch)) {
        if (pendingHigh_ == 0) return;   // 짝이 없는 뒤 글자는 버림

        InsertAtCaret(std::wstring{ pendingHigh_, ch });
        pendingHigh_ = 0;
    }
    else if (ch >= 0x20 && ch != 0x7F) {
        pendingHigh_ = 0;
        InsertAtCaret(std::wstring(1, ch));
    }
    else {
        return;   // Enter, Tab 같은 제어 문자는 여기서 처리하지 않음
    }

    GameWindow_Invalidate();
}

void ChatUI::OnComposition(const std::wstring& result, const std::wstring& composing)
{
    if (!open_) return;

    // 확정된 글자는 커서 위치에 넣고, 조합 중인 글자는 확정될 때까지 따로 보관
    if (!result.empty()) InsertAtCaret(result);
    composing_ = composing;

    GameWindow_Invalidate();
}

void ChatUI::OnKeyDown(WPARAM key)
{
    if (!open_) return;

    // 한글 조합 중에는 IME가 먼저 글자를 확정하므로 이동하지 않음
    if (!composing_.empty()) return;

    if (key == VK_LEFT && caret_ > 0) {
        --caret_;
        if (caret_ > 0 && IsLowSurrogate(draft_[caret_]) && IsHighSurrogate(draft_[caret_ - 1])) --caret_;
    }
    else if (key == VK_RIGHT && caret_ < draft_.size()) {
        if (IsHighSurrogate(draft_[caret_]) && caret_ + 1 < draft_.size()) ++caret_;
        ++caret_;
    }
    else {
        return;
    }

    GameWindow_Invalidate();
}

bool ChatUI::OnEnterKey()
{
    if (!open_ || draft_.empty()) return false;

    // 파티 대기 중이거나 전투 중이고 참가 중인 파티가 있을 때만 채팅 전송
    const ScreenState screen = state_.Screen();
    if ((screen != ScreenState::PartyWaiting && screen != ScreenState::Battle) || !state_.InParty()) return false;

    ChatSend message;
    message.text = WideToUtf8(draft_);
    message.textLength = static_cast<uint16_t>(message.text.size());

    const uint32_t requestId = nextRequestId_();
    const PacketCodec::Packet packet = PacketCodec::Encode(message, requestId);
    if (packet.empty() || !send_(packet)) {
        printf("[오류] CHAT_SEND 전송 실패\n");
        return false;   // 보내지 못했으므로 입력 내용은 그대로 둠
    }

    printf("[송신] CHAT_SEND 본문 %zu바이트, 패킷 전체 %zu바이트, requestId=%u: %s\n",
        message.text.size(), packet.size(), requestId, WideToConsole(draft_).c_str());

    draft_.clear();
    caret_ = 0;
    GameWindow_Invalidate();
    return true;
}

void ChatUI::Draw(HDC hdc, const std::vector<ChatMessage>& chatLog)
{
    LoadBitmaps();

    const RECT icon = ToggleButtonRect();
    DrawTransparentBitmap(hdc, iconBitmap_, icon.left, icon.top);

    if (!open_) return;

    const RECT window = WindowRect();
    DrawTransparentBitmap(hdc, windowBitmap_, window.left, window.top);

    // 입력 중인 문장, 커서 위치에 조합 중인 글자를 끼워서 표시
    const RECT input = {
        window.left + kInputLeft + kInputPadding, window.top + kInputTop,
        window.left + kInputLeft + kInputWidth - kInputPadding, window.top + kInputTop + kInputHeight };
    DrawDraft(hdc, input, draft_.substr(0, caret_), composing_, draft_.substr(caret_));

    // 메시지 목록은 서버가 채팅을 방송하는 작업 후, 말풍선 이미지 위에 이름과 본문으로 표시
    (void)chatLog;
}
