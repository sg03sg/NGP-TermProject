#include "ClientState.h"
#include <algorithm>

void ClientState::SetMyPlayerId(PlayerId id)
{
    myPlayerId_ = id;
    screen_ = ScreenState::Lobby;
}

void ClientState::ApplyPartyState(const PartySnapshot& snap)
{
    // 내가 없는 명단은 이미 나간 파티의 늦게 도착한 상태이므로 무시
    if (myPlayerId_ != 0 && !snap.members.empty()) {
        const bool hasMe = std::any_of(snap.members.begin(), snap.members.end(), [this](const PartyMemberInfo& m) { return m.playerId == myPlayerId_; });
        if (!hasMe) return;
    }

    party_ = snap;

    // 로비/대기창 단계에서만 화면을 바꾼다.
    if (screen_ == ScreenState::Lobby || screen_ == ScreenState::PartyWaiting)
        screen_ = snap.members.empty() ? ScreenState::Lobby : ScreenState::PartyWaiting;
}

void ClientState::ApplyLobbyState(const std::vector<LobbyMapInfo>& maps)
{
    lobbyMaps_ = maps;   // 주기 방송이므로 항상 통째로 교체
}

void ClientState::Clear()
{
    // 로비 복귀: myPlayerId_와 lobbyMaps_는 유지
    party_ = PartySnapshot{};
    battle_ = BattleInit{};
    chatLog_.clear();   // 파티를 떠나면 채팅 내역도 비움
    screen_ = ScreenState::Lobby;
}

void ClientState::ApplyBattleInit(const BattleInit& message)
{
    battle_ = message;
    screen_ = ScreenState::Battle;
}

void ClientState::AppendChat(const ChatEntry& entry)
{
    chatLog_.push_back(entry);
    if (chatLog_.size() > kMaxChatLog)
        chatLog_.erase(chatLog_.begin());
}