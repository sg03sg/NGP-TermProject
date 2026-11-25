#pragma once

#include <functional>
#include <string>
#include <vector>
#include "Command.h"
#include "PacketCodec.h"
#include "PartyManager.h"

class PartyCommandHandler {
public:
    using Packet = PacketCodec::Packet;

    struct Context {
        PartyManager& parties;                                                          // GameServer가 가진 PartyManager
        std::function<void(PlayerId, const Packet&)>                  sendToPlayer;     // 한 명에게 전송
        std::function<void(const std::vector<PlayerId>&, const Packet&)> sendToPlayers; // 여러 명에게 전송
        std::function<std::string(PlayerId)>                          nameOf;           // 닉네임 조회 (players_)
        std::function<void()>                                         onLobbyChanged;   // 로비 현황이 바뀜
    };

    explicit PartyCommandHandler(Context ctx);

    void HandleJoinMap(const Command& cmd, PlayerId pid);      // 맵 참가와 파티 생성
    void HandleLeaveParty(const Command& cmd, PlayerId pid);   // 퇴장 승인, 방장 이전, 빈 파티 삭제
    void HandlePlayerLeft(PlayerId pid);                       // 연결이 끊긴 플레이어를 파티에서 제거

private:
    void SendRejected(PlayerId pid, MessageType rejectedType, PartyError error, uint32_t requestId);
    void BroadcastPartyState(PartyId partyId);                 // 파티원 전원에게 PARTY_STATE
    void SendChatHistory(PlayerId pid, PartyId partyId);       // 새 참가자에게만 지금까지의 채팅 내역을 전송

    Context ctx_;
};