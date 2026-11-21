#include "ClientState.h"

void ClientState::SetMyPlayerId(PlayerId id)
{
    myPlayerId_ = id;
    screen_ = ScreenState::Lobby;
}

void ClientState::ApplyPartyState(const PartySnapshot& snap)
{
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
    party_ = PartySnapshot{};
    battle_ = BattleInit{};
    screen_ = ScreenState::Lobby;
}

void ClientState::ApplyBattleInit(const BattleInit& message)
{
    battle_ = message;
    screen_ = ScreenState::Battle;
}