#pragma once
#include "LobbyScreen.h"
#include "ChatUI.h"
#include "../Unit.h"

// 마우스 클릭이 어느 화면의 어느 요소에서 일어났는지 한 곳에서 판별하고 해당 UI의 처리 함수를 호출
// 로비의 맵 선택, 파티 대기창의 버튼, 전투 화면의 병종 선택과 소환 요청을 처리
class InputController {
public:
    using SendFn = MapUI::SendFn;               // 서버로 패킷 전송
    using RequestIdFn = MapUI::RequestIdFn;     // 요청 번호 발급

    InputController(LobbyScreen& lobby, ChatUI& chat, SendFn send, RequestIdFn nextRequestId);

    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    void OnLButtonDown(int x, int y);   // 창의 왼쪽 클릭 좌표를 받아 현재 화면 단계에 맞게 분배
    void OnChar(wchar_t ch);            // 키보드로 입력된 글자를 채팅창이 열려 있을 때 채팅 UI로 전달
    void OnKeyDown(WPARAM key);         // 방향키 같은 글자가 아닌 키를 채팅창이 열려 있을 때 채팅 UI로 전달
    void OnImeComposition(const std::wstring& result, const std::wstring& composing);   // 한글 IME의 확정된 글자와 조합 중인 글자를 채팅 UI로 전달
    void SelectUnit(UnitType type);     // 선택된 병종 기록
    void PaintOverlay(HDC hdc);         // 전투 화면의 병종 UI 위에 선택된 병종 표시

private:
    bool IsChatInputActive() const;           // 파티 대기창에서 채팅창이 열려 키 입력을 받는 상태인지
    void OnLobbyClick(const POINT& point);    // 로비 화면에서 문 위 클릭이면 해당 맵 선택
    void OnPartyClick(const POINT& point);    // 파티 대기창에서 시작·나가기 버튼 위 클릭이면 해당 요청
    void OnBattleClick(const POINT& point);   // 전투 화면에서 병종 UI 클릭과 전장 클릭을 구분
    bool BuildSpawnRequest(UnitType type, const POINT& pos, PacketCodec::Packet& packet);   // 클릭한 좌표로 소환 요청 생성

    LobbyScreen& lobby_;
    ChatUI&      chat_;
    SendFn       send_;
    RequestIdFn  nextRequestId_;
    UnitType     selectedType_ = UnitType::Barbarian;   // 선택된 병종
};
