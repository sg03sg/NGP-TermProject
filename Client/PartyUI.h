#pragma once
#include <windows.h>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include "../PacketCodec.h"
#include "ClientState.h"
#include "MapUI.h"

class PartyUI {
public:
    using SendFn = MapUI::SendFn;
    using RequestIdFn = MapUI::RequestIdFn;

    PartyUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId);
    ~PartyUI();

    PartyUI(const PartyUI&) = delete;
    PartyUI& operator=(const PartyUI&) = delete;


    void Draw(HDC hdc, const PartyState& partyState);   // 참가자 명단 표시
    bool OnStartClicked();                              // 시작 요청 전송, 방장만
    bool OnLeaveClicked();                              // 퇴장 요청 전송

    static RECT PanelRect();                                  // 파티 창 이미지의 화면 좌표, 채팅창 배치의 기준
    static RECT StartButtonRect();                            // 시작 버튼의 화면 좌표, 클릭 판별은 InputController가 사용
    static RECT LeaveButtonRect();                            // 나가기 버튼의 화면 좌표
    void OnLeaveApproved();                                   // 퇴장 승인 수신
    void OnRejected(uint32_t requestId, uint16_t errorCode);  // 서버가 시작·퇴장을 거절
    void Reset();

private:
    void LoadBitmaps();                      // 리소스의 파티 창 이미지를 처음 그릴 때 한 번 로드

    const ClientState& state_;
    SendFn             send_;
    RequestIdFn        nextRequestId_;
    uint32_t           pendingLeaveId_ = 0;   // 응답을 기다리는 퇴장 요청 번호
    std::wstring       message_;
    HBITMAP            panelBitmap_ = nullptr;      // 파티 창 이미지
    HBITMAP            startOffBitmap_ = nullptr;   // 방장이 아닐 때 시작 버튼 자리에 덮는 회색 버튼 이미지
};