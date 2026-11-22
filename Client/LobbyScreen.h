#pragma once
#include <windows.h>
#include <cstdint>
#include "../PacketCodec.h"
#include "ClientState.h"
#include "MapUI.h"
#include "PartyUI.h"

class LobbyScreen {
public:
    LobbyScreen(MapUI::SendFn send, MapUI::RequestIdFn nextRequestId);

    LobbyScreen(const LobbyScreen&) = delete;
    LobbyScreen& operator=(const LobbyScreen&) = delete;

    void SetWindow(HWND hWnd) { hWnd_ = hWnd; }           // 상태가 바뀌면 이 창을 다시 그리게 함
    void OnWelcome(uint32_t playerId);                    // WELCOME 수신: 내 ID 저장, 로비 화면으로
    bool HandlePacket(const PacketCodec::Packet& packet); // PARTY_STATE, LEAVE_APPROVED, REQUEST_REJECT 처리. 처리했으면 true
    bool OnLButtonDown(int x, int y);                     // 파티 대기창의 버튼 클릭 처리. 버튼 위였으면 true
    bool OnMapClicked(uint8_t mapId);                     // 게임 화면에서 고른 맵의 참가 요청 전송
    void OnBattleInit(const BattleInit& message);         // BATTLE_INIT 수신: 전투 상태로 전환
    void Paint(HWND hWnd, HDC hdc);                       // 현재 화면을 백버퍼에 그려 hdc로 복사
    void PaintOverlay(HDC hdc);                           // 기존 게임 화면 위에 파티 대기창과 안내 문구만 얹어 그림
    bool IsPartyVisible() const;                          // 파티 대기창이 떠 있는지

    bool IsActive() const;                                // 로비·파티 대기창을 보여줄 단계인지
    const ClientState& State() const { return state_; }

private:
    std::vector<LobbyMapInfo> MapList() const;
    void Invalidate() const;
    bool HandlePacketImpl(const PacketCodec::Packet& packet);

    HWND        hWnd_ = nullptr;

    ClientState state_;
    MapUI       mapUI_;
    PartyUI     partyUI_;
};