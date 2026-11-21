#pragma once
#include <windows.h>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include "../PacketCodec.h"
#include "../ClientState.h"
#include "MapUI.h"

class PartyUI {
public:
    using SendFn = MapUI::SendFn;
    using RequestIdFn = MapUI::RequestIdFn;

    PartyUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId);

    
    void Draw(HDC hdc, const PartyState& partyState);   // 참가자 명단 표시
    bool OnStartClicked();                              // 시작 요청 전송, 방장만
    bool OnLeaveClicked();                              // 퇴장 요청 전송

    
    bool HandleClick(int x, int y);                           // 마우스 좌표가 시작·나가기 버튼 위인지 판별
    void OnLeaveApproved();                                   // 퇴장 승인 수신
    void OnRejected(uint32_t requestId, uint16_t errorCode);  // 서버가 시작·퇴장을 거절
    void Reset();

private:
    static RECT StartButtonRect();
    static RECT LeaveButtonRect();

    const ClientState& state_;
    SendFn             send_;
    RequestIdFn        nextRequestId_;
    uint32_t           pendingLeaveId_ = 0;   // 응답을 기다리는 퇴장 요청 번호
    std::wstring       message_;
};