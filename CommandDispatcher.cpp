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

CommandDispatcher::CommandDispatcher(GameServer& server) : server_(server) {
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

void CommandDispatcher::HandleHello(const Command& cmd)         { LogReceived(cmd, "HELLO"); }
void CommandDispatcher::HandleJoinMap(const Command& cmd)       { LogReceived(cmd, "JOIN_MAP"); }
void CommandDispatcher::HandleStartGame(const Command& cmd)     { LogReceived(cmd, "START_GAME"); }
void CommandDispatcher::HandleLeaveParty(const Command& cmd)   { LogReceived(cmd, "LEAVE_PARTY"); }
void CommandDispatcher::HandleChatSend(const Command& cmd)      { LogReceived(cmd, "CHAT_SEND"); }
void CommandDispatcher::HandleSpawn(const Command& cmd)         { LogReceived(cmd, "SPAWN_REQUEST"); }
void CommandDispatcher::HandleAbortVoteCast(const Command& cmd) { LogReceived(cmd, "ABORT_VOTE_CAST"); }
void CommandDispatcher::HandleMarkerRequest(const Command& cmd) { LogReceived(cmd, "MARKER_REQUEST"); }

void CommandDispatcher::HandleDisconnected(int sessionId)
{
    std::cout << "[CommandDispatcher] 세션 " << sessionId << ": 연결 종료 이벤트 수신" << std::endl;
}
