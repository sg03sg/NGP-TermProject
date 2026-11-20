#pragma once
#include <cstdint>
#include <vector>
#include "LobbyState.h"

class SessionManager;

class LobbyBroadcaster {
private:
    uint32_t intervalMs_;
    uint64_t lastSentMs_ = 0;
    bool     sendNext_ = true;   // true면 주기와 관계없이 다음 Tick에서 바로 전송

public:
    explicit LobbyBroadcaster(uint32_t intervalMs = 1000);

    // WorkerThread의 매 틱마다 호출. 주기가 되었고 대상이 있으면 전송하고 true.
    bool Tick(uint64_t nowMs,
        const std::vector<LobbyMapInfo>& maps,
        const std::vector<int>& lobbySessionIds,
        SessionManager& sessions);

    
    bool ShouldSend(uint64_t nowMs) const { return sendNext_ || nowMs - lastSentMs_ >= intervalMs_; }

    // 접속·참가·퇴장·전투 종료처럼 현황이 바뀌었을 때 호출하면 다음 Tick에서 즉시 방송
    void Invalidate() { sendNext_ = true; }
};