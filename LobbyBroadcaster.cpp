#include "LobbyBroadcaster.h"
#include "SessionManager.h"

LobbyBroadcaster::LobbyBroadcaster(uint32_t intervalMs)
    : intervalMs_(intervalMs)
{
}

bool LobbyBroadcaster::Tick(uint64_t nowMs,
    const std::vector<LobbyMapInfo>& maps,
    const std::vector<int>& lobbySessionIds,
    SessionManager& sessions)
{
    if (!sendNext_ && nowMs - lastSentMs_ < intervalMs_)
        return false;

    // 로비에 아무도 없으면 보낼 필요가 없다. 다음에 누가 들어오면 바로 보내도록 표시만 남긴다.
    if (lobbySessionIds.empty()) {
        sendNext_ = true;
        return false;
    }

    const std::vector<char> packet = EncodeLobbyStatePacket(maps);   // requestId = 0 (주기 방송)
    sessions.SendToSessions(lobbySessionIds, packet.data(), static_cast<int>(packet.size()));

    lastSentMs_ = nowMs;
    sendNext_ = false;
    return true;
}