#pragma once
#include <cstdint>
#include <string>
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

// 채팅창에 보여줄 한 줄, senderId가 0이면 서버가 보낸 것이 아니라 클라이언트가 만든 알림
struct ChatEntry {
    PlayerId    senderId = 0;
    std::string name;    // 보낸 사람 이름, UTF-8
    std::string text;    // 본문, UTF-8
};

constexpr size_t kMaxChatLog = 100;   // 클라이언트가 보관하는 채팅 줄 수, 서버가 보관하는 내역 수와 같음
class ClientState {
private:
    // 로비·파티 ----
    PlayerId                  myPlayerId_ = 0;                   // 내 식별자
    PartySnapshot             party_;                            // 파티 명단 상태
    std::vector<LobbyMapInfo> lobbyMaps_;                        // 로비 스테이지 현황
    ScreenState               screen_ = ScreenState::Connecting; // 현재 화면 단계
    BattleInit                battle_;



    // 채팅
    std::vector<ChatEntry>    chatLog_;                          // 받은 채팅 내역

    // 전투 상태·결과



public:
    //로비·파티 ----
    void SetMyPlayerId(PlayerId id);                              // WELCOME: 내 ID 저장, 로비 진입
    void ApplyPartyState(const PartySnapshot& snap);              // 명단과 방장 교체
    void ApplyLobbyState(const std::vector<LobbyMapInfo>& maps);  // 로비 현황 교체
    void ApplyBattleInit(const BattleInit& message);
    void AppendChat(const ChatEntry& entry);                      // 채팅 한 줄 추가, 넘치면 오래된 것부터 삭제
    void EnterResult() { screen_ = ScreenState::Result; }
    const BattleInit& Battle() const { return battle_; }
    const std::vector<ChatEntry>& ChatLog() const { return chatLog_; }
    void Clear();                                                 // 로비 복귀 시 초기화


    PlayerId                         MyPlayerId() const { return myPlayerId_; }
    ScreenState                      Screen()     const { return screen_; }
    const PartySnapshot& Party()      const { return party_; }
    const std::vector<LobbyMapInfo>& LobbyMaps()  const { return lobbyMaps_; }
    bool InParty() const { return !party_.members.empty(); }
    bool IsPartyLeader() const { return InParty() && party_.leaderId == myPlayerId_; }

    



};