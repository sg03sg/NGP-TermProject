#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include "../Message.h"
#include "../PacketCodec.h"
#include "ClientState.h"
#include "MapUI.h"

// 파티 대기창 옆의 채팅 UI
// 아이콘을 누르면 파티 창 오른쪽에 채팅창이 열리고 닫힌다
class ChatUI {
public:
    using SendFn = MapUI::SendFn;               // 서버로 패킷 전송
    using RequestIdFn = MapUI::RequestIdFn;     // 요청 번호 발급

    ChatUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId);
    ~ChatUI();

    ChatUI(const ChatUI&) = delete;
    ChatUI& operator=(const ChatUI&) = delete;

    void Toggle();                                                 // 창을 열고 닫는다
    bool OnEnterKey();                                             // 입력 중인 문장을 서버로 보내고 비운다
    void OnChar(wchar_t ch);                                       // 확정된 글자를 커서 위치에 더하고 백스페이스는 커서 앞 글자를 지운다
    void OnComposition(const std::wstring& result, const std::wstring& composing);   // 한글 IME 입력, 확정된 글자는 커서 위치에 더하고 조합 중인 글자는 따로 보관
    void OnKeyDown(WPARAM key);                                    // 좌우 방향키로 커서 이동
    void Draw(HDC hdc, const std::vector<ChatMessage>& chatLog);   // 아이콘과 열린 채팅창 표시, 메시지 목록 표시는 서버 채팅 방송 구현 후 추가

    bool IsOpen() const { return open_; }
    RECT ToggleButtonRect() const;   // 채팅 아이콘의 화면 좌표, 파티 대기창과 전투 화면에서 위치가 다름
    RECT WindowRect() const;         // 열린 채팅창의 화면 좌표, 파티 대기창과 전투 화면에서 위치가 다름

private:
    void LoadBitmaps();                          // 리소스의 이미지를 처음 그릴 때 한 번 로드
    void InsertAtCaret(const std::wstring& text);   // 커서 위치에 글자를 끼워 넣고 커서를 뒤로 옮김, UTF-8 100바이트를 넘는 글자는 넣지 않음

    const ClientState& state_;
    SendFn             send_;
    RequestIdFn        nextRequestId_;
    bool               open_ = false;          // 창 열림 여부
    std::wstring       draft_;                 // 입력 중인 문장
    std::wstring       composing_;             // 한글 조합 중이라 아직 확정되지 않은 글자
    size_t             caret_ = 0;             // 커서 위치, draft_의 몇 번째 글자 앞인지
    wchar_t            pendingHigh_ = 0;       // 이모지처럼 두 글자로 오는 문자의 앞 글자, 뒤 글자가 올 때까지 보관
    HBITMAP            iconBitmap_ = nullptr;      // 채팅 아이콘 이미지
    HBITMAP            windowBitmap_ = nullptr;    // 채팅창 이미지
    HBITMAP            bubbleBitmap_ = nullptr;    // 메시지 한 개의 말풍선 이미지
};
