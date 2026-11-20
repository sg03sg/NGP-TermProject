#include "CommandDispatcher.h"
#include "GameServer.h"
#include "Protocol.h"
#include <iostream>

namespace {
    // 패킷 헤더의 messageType
    uint16_t MessageTypeOf(const std::vector<uint8_t>& packet)
    {
        if (packet.size() < PACKET_HEADER_SIZE) return 0;
        return static_cast<uint16_t>((packet[4] << 8) | packet[5]);
    }

    // 처리 함수를 구현하기 전까지 수신 사실만 로그로 남김
    void LogReceived(const Command& cmd, const char* name)
    {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": " << name
            << " 수신 " << cmd.packet.size() << "바이트, 처리 함수 미구현" << std::endl;
    }
}

CommandDispatcher::CommandDispatcher(GameServer& server)
    : server_(server),
    partyHandler_(PartyCommandHandler::Context{
        server.GetPartyManager(),
        [this](PlayerId pid, const PacketCodec::Packet& p) { SendToPlayer(pid, p); },
        [this](const std::vector<PlayerId>& pids, const PacketCodec::Packet& p) { SendToPlayers(pids, p); },
        [this](PlayerId pid) { auto it = players_.find(pid); return it != players_.end() ? it->second : std::string(); },
        [&server]() { server.GetLobbyBroadcaster().Invalidate(); } })
{
}

void CommandDispatcher::SendToSession(int sessionId, const PacketCodec::Packet& packet)
{
    SendToSessions({ sessionId }, packet);
}

void CommandDispatcher::SendToSessions(const std::vector<int>& sessionIds, const PacketCodec::Packet& packet)
{
    if (packet.empty() || sessionIds.empty()) return;
    server_.GetSessionManager().SendToSessions(sessionIds, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()));
}

void CommandDispatcher::SendToPlayer(PlayerId pid, const PacketCodec::Packet& packet)
{
    auto it = playerToSession_.find(pid);
    if (it != playerToSession_.end()) SendToSession(it->second, packet);
}

void CommandDispatcher::SendToPlayers(const std::vector<PlayerId>& pids, const PacketCodec::Packet& packet)
{
    std::vector<int> ids;
    for (PlayerId pid : pids) {
        auto it = playerToSession_.find(pid);
        if (it != playerToSession_.end()) ids.push_back(it->second);
    }
    SendToSessions(ids, packet);
}

PlayerId CommandDispatcher::PlayerOf(int sessionId) const
{
    auto it = sessionToPlayer_.find(sessionId);
    return it == sessionToPlayer_.end() ? kInvalidPlayerId : it->second;
}

void CommandDispatcher::Dispatch(const Command& cmd)
{
    switch (cmd.type) {
    case Command::Type::Packet:
        DispatchPacket(cmd);
        break;
    case Command::Type::Disconnected:
        HandleDisconnected(cmd.sessionId);
        break;
    }
}

void CommandDispatcher::DispatchPacket(const Command& cmd)
{
    const uint16_t type = MessageTypeOf(cmd.packet);

    switch (static_cast<MessageType>(type)) {
    case MessageType::HELLO:            HandleHello(cmd);           break;
    case MessageType::JOIN_MAP:         HandleJoinMap(cmd);         break;
    case MessageType::START_GAME:       HandleStartGame(cmd);       break;
    case MessageType::LEAVE_PARTY:      HandleLeaveParty(cmd);      break;
    case MessageType::CHAT_SEND:        HandleChatSend(cmd);        break;
    case MessageType::SPAWN_REQUEST:    HandleSpawn(cmd);           break;
    case MessageType::ABORT_VOTE_CAST:  HandleAbortVoteCast(cmd);   break;
    case MessageType::MARKER_REQUEST:   HandleMarkerRequest(cmd);   break;

    default:
        // 서버가 보내는 종류이거나 정의되지 않은 종류
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": 처리하지 않는 메시지 종류 "
            << type << " " << cmd.packet.size() << "바이트" << std::endl;
        break;
    }
}

void CommandDispatcher::HandleHello(const Command& cmd)
{
    Hello hello;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, hello, &header)) {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 디코딩 실패" << std::endl;
        return;
    }
    if (PlayerOf(cmd.sessionId) != kInvalidPlayerId) return;   // 중복 HELLO 무시

    const PlayerId pid = nextPlayerId_++;
    sessionToPlayer_[cmd.sessionId] = pid;
    playerToSession_[pid] = cmd.sessionId;
    players_[pid] = hello.name;
    std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO \"" << hello.name << "\" -> 플레이어 " << pid << std::endl;

    Welcome welcome;
    welcome.playerId = pid;
    SendToSession(cmd.sessionId, PacketCodec::Encode(welcome, header.requestId));
    server_.GetLobbyBroadcaster().Invalidate();
}

void CommandDispatcher::HandleJoinMap(const Command& cmd)
{
    const PlayerId pid = PlayerOf(cmd.sessionId);
    if (pid == kInvalidPlayerId) { std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 전 JOIN_MAP 무시" << std::endl; return; }
    partyHandler_.HandleJoinMap(cmd, pid);
}
void CommandDispatcher::HandleStartGame(const Command& cmd) { LogReceived(cmd, "START_GAME"); }
void CommandDispatcher::HandleLeaveParty(const Command& cmd)
{
    const PlayerId pid = PlayerOf(cmd.sessionId);
    if (pid == kInvalidPlayerId) { std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 전 LEAVE_PARTY 무시" << std::endl; return; }
    partyHandler_.HandleLeaveParty(cmd, pid);
}
void CommandDispatcher::HandleChatSend(const Command& cmd) { LogReceived(cmd, "CHAT_SEND"); }
void CommandDispatcher::HandleSpawn(const Command& cmd) { LogReceived(cmd, "SPAWN_REQUEST"); }
void CommandDispatcher::HandleAbortVoteCast(const Command& cmd) { LogReceived(cmd, "ABORT_VOTE_CAST"); }
void CommandDispatcher::HandleMarkerRequest(const Command& cmd)
{
    MarkerRequest req;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, req, &header)) {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": MARKER_REQUEST 디코딩 실패" << std::endl;
        return;
    }

    // HELLO 전이면 세션 번호를 발신자로 쓴다 (통합 테스트용)
    const PlayerId pid = PlayerOf(cmd.sessionId);
    MarkerPlaced placed;
    placed.senderId = pid != kInvalidPlayerId ? pid : static_cast<uint32_t>(cmd.sessionId);
    placed.x = req.x;
    placed.y = req.y;
    const PacketCodec::Packet packet = PacketCodec::Encode(placed, 0);

    // 파티가 있으면 파티원 전체, 없으면 접속 중인 모두에게 보낸다 (보낸 사람 포함)
    PartyManager& parties = server_.GetPartyManager();
    if (pid != kInvalidPlayerId && parties.IsInParty(pid)) {
        SendToPlayers(parties.MembersOf(parties.PartyOf(pid)), packet);
    }
    else {
        SendToSessions(server_.GetSessionManager().GetSessionIds(), packet);
    }
}

void CommandDispatcher::HandleDisconnected(int sessionId)
{
    std::cout << "[CommandDispatcher] 세션 " << sessionId << ": 연결 종료 이벤트 수신" << std::endl;

    auto it = sessionToPlayer_.find(sessionId);
    if (it == sessionToPlayer_.end()) return;   // HELLO 전에 끊김
    const PlayerId pid = it->second;

    partyHandler_.HandlePlayerLeft(pid);        // 파티에서 제거, 남은 파티원에게 PARTY_STATE
    sessionToPlayer_.erase(it);
    playerToSession_.erase(pid);
    players_.erase(pid);
    server_.GetLobbyBroadcaster().Invalidate();
}