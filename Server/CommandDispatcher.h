#pragma once
#include <map>
#include <string>
#include <vector>
#include "Command.h"
#include "PacketCodec.h"
#include "PartyCommandHandler.h"

class GameServer;

// 요청 큐에서 꺼낸 요청을 messageType별 처리 함수로 분배
// 워커 스레드에서만 호출
class CommandDispatcher {
private:
    GameServer& server_;

    // 접속 상태: sessionId <-> playerId. 워커 스레드만 접근하므로 락이 없다.
    std::map<int, PlayerId>         sessionToPlayer_;
    std::map<PlayerId, int>         playerToSession_;
    std::map<PlayerId, std::string> players_;   // playerId -> 닉네임
    PlayerId                        nextPlayerId_ = 1;

    PartyCommandHandler             partyHandler_;   // 장현우: 로비·파티

    // 전송 보조 (인코딩 실패로 빈 패킷이면 보내지 않는다)
    void SendToSession(int sessionId, const PacketCodec::Packet& packet);
    void SendToSessions(const std::vector<int>& sessionIds, const PacketCodec::Packet& packet);
    void SendToPlayer(PlayerId pid, const PacketCodec::Packet& packet);
    void SendToPlayers(const std::vector<PlayerId>& pids, const PacketCodec::Packet& packet);
    PlayerId PlayerOf(int sessionId) const;   // HELLO 전이면 kInvalidPlayerId

    void DispatchPacket(const Command& cmd);

    // 접속
    void HandleHello(const Command& cmd);
    void HandleDisconnected(int sessionId);

    // 로비와 파티
    void HandleJoinMap(const Command& cmd);
    void HandleStartGame(const Command& cmd);
    void HandleLeaveParty(const Command& cmd);

    // 채팅
    void HandleChatSend(const Command& cmd);

    // 전투
    void HandleSpawn(const Command& cmd);
    void HandleAbortVoteCast(const Command& cmd);

    // 좌표 테스트
    void HandleMarkerRequest(const Command& cmd);

public:
    explicit CommandDispatcher(GameServer& server);

    void Dispatch(const Command& cmd);

    void SendBattleState(const BattleState& message);

    void SendBattleProgress(const BattleProgress& message);
};