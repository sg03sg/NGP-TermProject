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
    // 로비 복귀: myPlayerId_와 lobbyMaps_는 유지
    party_ = PartySnapshot{};
    screen_ = ScreenState::Lobby;
    



}