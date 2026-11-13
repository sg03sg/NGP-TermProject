#pragma once

#include <cstdint>
#include <vector>

#include "Protocol.h"
#include "Message.h"

using namespace std;

class PacketCodec {
public:
    using Packet = vector<uint8_t>;

    // ==============================================================================================
    // 인코드
    // ==============================================================================================

    // 접속 및 맵 참가
    static Packet Encode(const Hello& message, uint32_t requestId = 0);
    static Packet Encode(const Welcome& message, uint32_t requestId = 0);
    static Packet Encode(const JoinMap& message, uint32_t requestId = 0);

    // 파티
    static Packet Encode(const PartyState& message, uint32_t requestId = 0);
    static Packet Encode(const StartGame& message, uint32_t requestId = 0);
    static Packet Encode(const LeaveParty& message, uint32_t requestId = 0);
    static Packet Encode(const LeaveApproved& message, uint32_t requestId = 0);

    // 채팅
    static Packet Encode(const ChatSend& message, uint32_t requestId = 0);
    static Packet Encode(const ChatMessage& message, uint32_t requestId = 0);

    // 전투 초기화 및 소환
    static Packet Encode(const BattleInit& message, uint32_t requestId = 0);
    static Packet Encode(const SpawnRequest& message, uint32_t requestId = 0);
    static Packet Encode(const UnitCreated& message, uint32_t requestId = 0);
    static Packet Encode(const ManaState& message, uint32_t requestId = 0);
    static Packet Encode(const RequestRejected& message, uint32_t requestId = 0);

    // 전투 진행 및 결과
    static Packet Encode(const BattleState& message, uint32_t requestId = 0);
    static Packet Encode(const BattleProgress& message, uint32_t requestId = 0);
    static Packet Encode(const BattleResult& message, uint32_t requestId = 0);
    static Packet Encode(const ReturnLobby& message, uint32_t requestId = 0);

    // 중단 투표
    static Packet Encode(const AbortVoteStart& message, uint32_t requestId = 0);
    static Packet Encode(const AbortVoteCast& message, uint32_t requestId = 0);
    static Packet Encode(const AbortVoteResult& message, uint32_t requestId = 0);

    // 좌표 테스트
    static Packet Encode(const MarkerRequest& message, uint32_t requestId = 0);
    static Packet Encode(const MarkerPlaced& message, uint32_t requestId = 0);

    // ==============================================================================================
    // 디코드 
    // ==============================================================================================
    
    // 접속 및 맵 참가
    static bool Decode(const Packet& packet, Hello& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, Welcome& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, JoinMap& out, PacketHeader* header = nullptr);

    // 파티
    static bool Decode(const Packet& packet, PartyState& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, StartGame& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, LeaveParty& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, LeaveApproved& out, PacketHeader* header = nullptr);

    // 채팅
    static bool Decode(const Packet& packet, ChatSend& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, ChatMessage& out, PacketHeader* header = nullptr);

    // 전투 초기화 및 소환
    static bool Decode(const Packet& packet, BattleInit& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, SpawnRequest& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, UnitCreated& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, ManaState& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, RequestRejected& out, PacketHeader* header = nullptr);

    // 전투 진행 및 결과
    static bool Decode(const Packet& packet, BattleState& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, BattleProgress& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, BattleResult& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, ReturnLobby& out, PacketHeader* header = nullptr);

    // 중단 투표
    static bool Decode(const Packet& packet, AbortVoteStart& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, AbortVoteCast& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, AbortVoteResult& out, PacketHeader* header = nullptr);

    // 좌표 테스트
    static bool Decode(const Packet& packet, MarkerRequest& out, PacketHeader* header = nullptr);
    static bool Decode(const Packet& packet, MarkerPlaced& out, PacketHeader* header = nullptr);
};