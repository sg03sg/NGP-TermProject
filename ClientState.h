#pragma once
#include <cstdint>
#include <vector>
#include "Message.h"       // PartyState
#include "LobbyState.h"    // LobbyMapInfo (LOBBY_STATE)

using PlayerId = uint32_t;

// 화면 단계
enum class ScreenState : uint8_t {
    Connecting,     // 연결·HELLO 진행 중
    Lobby,          // 스테이지 목록
    PartyWaiting,   // 파티 대기창
    Battle,         // 전투 화면
    Result          // 결과 화면
};

using PartySnapshot = PartyState;   // message.h의 PartyState

class ClientState {
private:
    // 로비·파티 ----
    PlayerId                  myPlayerId_ = 0;                   // 내 식별자
    PartySnapshot             party_;                            // 파티 명단 상태
    std::vector<LobbyMapInfo> lobbyMaps_;                        // 로비 스테이지 현황
    ScreenState               screen_ = ScreenState::Connecting; // 현재 화면 단계



    // 소환·마나·채팅

    // 전투 상태·결과



public:
    //로비·파티 ----
    void SetMyPlayerId(PlayerId id);                              // WELCOME: 내 ID 저장, 로비 진입
    void ApplyPartyState(const PartySnapshot& snap);              // 명단과 방장 교체
    void ApplyLobbyState(const std::vector<LobbyMapInfo>& maps);  // 로비 현황 교체
    void Clear();                                                 // 로비 복귀 시 초기화

    PlayerId                         MyPlayerId() const { return myPlayerId_; }
    ScreenState                      Screen()     const { return screen_; }
    const PartySnapshot& Party()      const { return party_; }
    const std::vector<LobbyMapInfo>& LobbyMaps()  const { return lobbyMaps_; }
    bool InParty() const { return !party_.members.empty(); }
    bool IsPartyLeader() const { return InParty() && party_.leaderId == myPlayerId_; }

    



};