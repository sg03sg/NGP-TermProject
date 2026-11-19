#pragma once
#include "Command.h"

class GameServer;

// 요청 큐에서 꺼낸 요청을 messageType별 처리 함수로 분배
// 워커 스레드에서만 호출
class CommandDispatcher {
private:
    GameServer& server_;

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
};
