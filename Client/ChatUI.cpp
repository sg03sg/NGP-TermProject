#include "ChatUI.h"
#include "GameWindow.h"
#include "LobbyUiCommon.h"
#include "PartyUI.h"
#include "../resource.h"
#include "../RendererGlobals.h"
#include <algorithm>
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

    // 채팅창 이미지 안쪽의 메시지 목록 영역
    constexpr int kLogLeft = 20;
    constexpr int kLogTop = 60;
    constexpr int kLogWidth = 320;
    constexpr int kLogHeight = 340;

    // 말풍선 이미지는 300x60, 세로로 늘려야 할 때 꼬리가 있는 위쪽과 모서리가 있는 아래쪽은 그대로 두고 가운데 몇 줄만 늘림
    constexpr int kBubbleWidth = 300;
    constexpr int kBubbleHeight = 60;
    constexpr int kBubbleTopRows = 37;
    constexpr int kBubbleStretchRows = 5;
    constexpr int kBubbleBottomRows = kBubbleHeight - kBubbleTopRows - kBubbleStretchRows;
    constexpr int kBubbleTextLeft = 20;     // 말풍선 왼쪽 꼬리와 여백을 뺀 글자 시작 위치
    constexpr int kBubbleTextWidth = 268;
    constexpr int kBubblePadding = 8;
    constexpr int kNameHeight = 18;
    constexpr int kEntryGap = 8;

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

    // 영역 너비에 맞춰 줄바꿈, 가능하면 공백에서 끊고 공백이 없는 긴 글은 글자 단위로 끊음
    std::vector<std::wstring> WrapText(HDC hdc, const std::wstring& text, int width)
    {
        std::vector<std::wstring> lines;
        size_t start = 0;

        while (start < text.size()) {
            int fit = 0;
            SIZE size = { 0, 0 };
            GetTextExtentExPointW(hdc, text.c_str() + start, static_cast<int>(text.size() - start), width, &fit, nullptr, &size);
            if (fit <= 0) fit = 1;   // 한 글자도 들어가지 않아도 반드시 진행

            size_t count = static_cast<size_t>(fit);
            bool atSpace = false;

            if (start + count < text.size()) {
                const size_t space = text.rfind(L' ', start + count);   // 들어가는 범위 안의 마지막 공백
                if (space != std::wstring::npos && space > start) {
                    count = space - start;
                    atSpace = true;
                }
                else if (count > 1 && IsHighSurrogate(text[start + count - 1])) {
                    --count;   // 이모지의 앞 글자만 윗줄에 남지 않도록 함
                }
            }

            lines.push_back(text.substr(start, count));
            start += count + (atSpace ? 1 : 0);
        }

        if (lines.empty()) lines.push_back(std::wstring());
        return lines;
    }

    // 말풍선 이미지를 원하는 높이로 그림, 마젠타를 투명색으로 사용
    void DrawBubble(HDC hdc, HBITMAP bubble, int x, int y, int height)
    {
        if (bubble == nullptr) return;

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bubble);

        const int stretchHeight = height - kBubbleTopRows - kBubbleBottomRows;
        TransparentBlt(hdc, x, y, kBubbleWidth, kBubbleTopRows, memDC, 0, 0, kBubbleWidth, kBubbleTopRows, kTransparentColor);
        if (stretchHeight > 0)
            TransparentBlt(hdc, x, y + kBubbleTopRows, kBubbleWidth, stretchHeight, memDC, 0, kBubbleTopRows, kBubbleWidth, kBubbleStretchRows, kTransparentColor);
        TransparentBlt(hdc, x, y + kBubbleTopRows + (stretchHeight > 0 ? stretchHeight : 0), kBubbleWidth, kBubbleBottomRows,
            memDC, 0, kBubbleTopRows + kBubbleStretchRows, kBubbleWidth, kBubbleBottomRows, kTransparentColor);

        SelectObject(memDC, oldBitmap);
        DeleteDC(memDC);
    }

    // 받은 채팅 내역을 최신 것이 아래에 오도록 영역 안에 말풍선으로 그림
    // 이름은 말풍선 위쪽에, 본문은 그 아래에 영역 너비에 맞춰 줄바꿈하며 보낸 사람이 나이면 이름 색을 다르게 표시
    // 서버가 보내는 알림처럼 보낸 사람이 없는 줄은 말풍선 없이 회색 글씨로 가운데에 표시
    void DrawChatLog(HDC hdc, const RECT& area, HBITMAP bubble, const std::vector<ChatEntry>& log, uint32_t myPlayerId)
    {
        if (log.empty()) return;

        HFONT textFont = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕");
        HFONT nameFont = CreateFontW(15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"맑은 고딕");
        HFONT oldFont = (HFONT)SelectObject(hdc, textFont);
        SetBkMode(hdc, TRANSPARENT);

        TEXTMETRICW metric;
        GetTextMetricsW(hdc, &metric);
        const int lineHeight = metric.tmHeight;

        const int areaWidth = area.right - area.left;
        const int areaHeight = area.bottom - area.top;

        struct Item {
            const ChatEntry* entry;
            std::wstring     name;
            std::wstring     text;
            std::vector<std::wstring> lines;   // 너비에 맞춰 나눈 본문
            int              textHeight;
            int              height;
            bool             notice;
        };

        // 최신 것부터 거꾸로 영역이 찰 때까지 높이를 계산해 모음
        std::vector<Item> items;
        int total = 0;
        for (size_t i = log.size(); i-- > 0 && total <= areaHeight; ) {
            Item item;
            item.entry = &log[i];
            item.notice = log[i].senderId == 0;
            item.name = Utf8ToWide(log[i].name);
            item.text = Utf8ToWide(log[i].text);

            const int width = item.notice ? areaWidth - 20 : kBubbleTextWidth;
            item.lines = WrapText(hdc, item.text, width);
            item.textHeight = static_cast<int>(item.lines.size()) * lineHeight;
            item.height = item.notice ? item.textHeight + 6 : (std::max)(kBubbleHeight, kBubblePadding * 2 + kNameHeight + item.textHeight);

            total += item.height + kEntryGap;
            items.push_back(std::move(item));
        }

        const int saved = SaveDC(hdc);
        IntersectClipRect(hdc, area.left, area.top, area.right, area.bottom);

        int y = area.bottom - kEntryGap;
        for (const Item& item : items) {
            y -= item.height;

            if (item.notice) {
                SelectObject(hdc, textFont);
                SetTextColor(hdc, RGB(150, 150, 150));
                for (size_t k = 0; k < item.lines.size(); ++k) {
                    RECT line = { area.left + 10, y + 3 + static_cast<int>(k) * lineHeight, area.right - 10, y + 3 + static_cast<int>(k + 1) * lineHeight };
                    DrawTextW(hdc, item.lines[k].c_str(), -1, &line, DT_CENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                }
            }
            else {
                const int x = area.left + (areaWidth - kBubbleWidth) / 2;
                DrawBubble(hdc, bubble, x, y, item.height);

                const bool mine = item.entry->senderId == myPlayerId;
                RECT nameRect = { x + kBubbleTextLeft, y + kBubblePadding, x + kBubbleTextLeft + kBubbleTextWidth, y + kBubblePadding + kNameHeight };
                SelectObject(hdc, nameFont);
                SetTextColor(hdc, mine ? RGB(255, 220, 120) : RGB(169, 187, 223));
                DrawTextW(hdc, item.name.c_str(), -1, &nameRect, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

                SelectObject(hdc, textFont);
                SetTextColor(hdc, RGB(255, 255, 255));
                for (size_t k = 0; k < item.lines.size(); ++k) {
                    const int lineTop = y + kBubblePadding + kNameHeight + static_cast<int>(k) * lineHeight;
                    RECT line = { x + kBubbleTextLeft, lineTop, x + kBubbleTextLeft + kBubbleTextWidth, lineTop + lineHeight };
                    DrawTextW(hdc, item.lines[k].c_str(), -1, &line, DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
                }
            }

            y -= kEntryGap;
        }

        RestoreDC(hdc, saved);
        SelectObject(hdc, oldFont);
        DeleteObject(textFont);
        DeleteObject(nameFont);
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

void ChatUI::Draw(HDC hdc, const std::vector<ChatEntry>& chatLog)
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

    // 받은 채팅 내역을 말풍선으로 표시
    const RECT logArea = {
        window.left + kLogLeft, window.top + kLogTop,
        window.left + kLogLeft + kLogWidth, window.top + kLogTop + kLogHeight };
    DrawChatLog(hdc, logArea, bubbleBitmap_, chatLog, state_.MyPlayerId());
}
