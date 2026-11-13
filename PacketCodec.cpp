#include "PacketCodec.h"

#include <string>

using namespace std;

namespace {

using Packet = PacketCodec::Packet;

// 기본 자료형 기록 ========================================================

void Write(Packet& out, uint8_t value)
{
    out.push_back(value);
}

void Write(Packet& out, uint16_t value)
{
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

void Write(Packet& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

// 여러 정수 필드를 적힌 순서대로 기록한다.
template <typename... Fields>
void WriteFields(Packet& out, const Fields&... fields)
{
    (Write(out, fields), ...);
}

// [문자열 길이 2바이트][문자열 바이트]
// 길이가 2바이트에 들어가지 않으면 false를 반환한다.
bool WriteString(Packet& out, const string& text)
{
    if (text.size() > UINT16_MAX) {
        return false;
    }

    Write(out, static_cast<uint16_t>(text.size()));
    out.insert(out.end(), text.begin(), text.end());
    return true;
}

// 모든 메시지가 공통으로 사용하는 패킷 조립 =====================================================

Packet BuildPacket(MessageType type, uint32_t requestId, const Packet& payload)
{
    if (payload.size() > UINT32_MAX - PACKET_HEADER_SIZE) {
        return {};
    }

    const uint32_t totalLength =
        PACKET_HEADER_SIZE + static_cast<uint32_t>(payload.size());

    Packet packet;
    packet.reserve(totalLength);

    Write(packet, totalLength);                   // 4바이트
    Write(packet, static_cast<uint16_t>(type));   // 2바이트
    Write(packet, requestId);                     // 4바이트

    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

} // namespace 끝

// 접속 및 맵 참가 ========================================================

Packet PacketCodec::Encode(const Hello& message, uint32_t requestId)
{
    Packet payload;

    if (!WriteString(payload, message.name)) {
        return {};
    }

    return BuildPacket(MessageType::HELLO, requestId, payload);
}

Packet PacketCodec::Encode(const Welcome& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.playerId);

    return BuildPacket(MessageType::WELCOME, requestId, payload);
}

Packet PacketCodec::Encode(const JoinMap& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.mapId);

    return BuildPacket(MessageType::JOIN_MAP, requestId, payload);
}

// 파티 =======================================================================

Packet PacketCodec::Encode(const PartyState& message, uint32_t requestId)
{
    if (message.members.size() > UINT8_MAX) {
        return {};
    }

    Packet payload;
    WriteFields(payload, message.partyId, message.mapId, message.leaderId);
    Write(payload, static_cast<uint8_t>(message.members.size()));

    for (const auto& member : message.members) {
        Write(payload, member.playerId);

        if (!WriteString(payload, member.name)) {
            return {};
        }
    }

    return BuildPacket(MessageType::PARTY_STATE, requestId, payload);
}

Packet PacketCodec::Encode(const StartGame& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.partyId);

    return BuildPacket(MessageType::START_GAME, requestId, payload);
}

Packet PacketCodec::Encode(const LeaveParty& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.partyId);

    return BuildPacket(MessageType::LEAVE_PARTY, requestId, payload);
}

Packet PacketCodec::Encode(const LeaveApproved& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.partyId);

    return BuildPacket(MessageType::LEAVE_APPROVED, requestId, payload);
}

// 채팅 =======================================================================

Packet PacketCodec::Encode(const ChatSend& message, uint32_t requestId)
{
    Packet payload;

    if (!WriteString(payload, message.text)) {
        return {};
    }

    return BuildPacket(MessageType::CHAT_SEND, requestId, payload);
}

Packet PacketCodec::Encode(const ChatMessage& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.senderId);

    if (!WriteString(payload, message.text)) {
        return {};
    }

    return BuildPacket(MessageType::CHAT_MESSAGE, requestId, payload);
}

// 전투 초기화 및 소환 =======================================================================

Packet PacketCodec::Encode(const BattleInit& message, uint32_t requestId)
{
    if (message.players.size() > UINT8_MAX ||
        message.structures.size() > UINT16_MAX) {
        return {};
    }

    Packet payload;
    WriteFields(payload, message.partyId, message.mapId, message.remainingSeconds);

    Write(payload, static_cast<uint8_t>(message.players.size()));
    for (const auto& player : message.players) {
        WriteFields(payload, player.playerId, player.mana, player.maxMana);
    }

    Write(payload, static_cast<uint16_t>(message.structures.size()));
    for (const auto& structure : message.structures) {
        WriteFields(payload, structure.structureId, structure.type,
                    structure.x, structure.y, structure.hp, structure.maxHp);
    }

    return BuildPacket(MessageType::BATTLE_INIT, requestId, payload);
}

Packet PacketCodec::Encode(const SpawnRequest& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.unitType, message.x, message.y);

    return BuildPacket(MessageType::SPAWN_REQUEST, requestId, payload);
}

Packet PacketCodec::Encode(const UnitCreated& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.unitId, message.ownerId, message.unitType,
                message.x, message.y, message.hp, message.maxHp);

    return BuildPacket(MessageType::UNIT_CREATED, requestId, payload);
}

Packet PacketCodec::Encode(const ManaState& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.mana, message.maxMana);

    return BuildPacket(MessageType::MANA_STATE, requestId, payload);
}

Packet PacketCodec::Encode(const RequestRejected& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.rejectedType, message.errorCode);

    return BuildPacket(MessageType::REQUEST_REJECT, requestId, payload);
}

// 전투 진행 및 결과 =======================================================================

Packet PacketCodec::Encode(const BattleState& message, uint32_t requestId)
{
    if (message.units.size() > UINT16_MAX ||
        message.structures.size() > UINT16_MAX) {
        return {};
    }

    Packet payload;
    WriteFields(payload, message.partyId, message.serverTick);

    Write(payload, static_cast<uint16_t>(message.units.size()));
    for (const auto& unit : message.units) {
        WriteFields(payload, unit.unitId, unit.ownerId, unit.unitType,
                    unit.x, unit.y, unit.hp, unit.maxHp, unit.action);
    }

    Write(payload, static_cast<uint16_t>(message.structures.size()));
    for (const auto& structure : message.structures) {
        WriteFields(payload, structure.structureId, structure.hp, structure.maxHp);
    }

    return BuildPacket(MessageType::BATTLE_STATE, requestId, payload);
}

Packet PacketCodec::Encode(const BattleProgress& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.partyId, message.remainingSeconds,
                message.destructionPercent, message.stars);

    return BuildPacket(MessageType::BATTLE_PROGRESS, requestId, payload);
}

Packet PacketCodec::Encode(const BattleResult& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.partyId, message.reason,
                message.destructionPercent, message.stars,
                message.returnSeconds);

    return BuildPacket(MessageType::BATTLE_RESULT, requestId, payload);
}

Packet PacketCodec::Encode(const ReturnLobby& message, uint32_t requestId)
{
    Packet payload;
    Write(payload, message.partyId);

    return BuildPacket(MessageType::RETURN_LOBBY, requestId, payload);
}

// 중단 투표 =======================================================================

Packet PacketCodec::Encode(const AbortVoteStart& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.partyId, message.leaverId, message.voteSeconds, message.requestedVotes, message.totalVoters);

    return BuildPacket(MessageType::ABORT_VOTE_START, requestId, payload);
}

Packet PacketCodec::Encode(const AbortVoteCast& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.partyId, message.stop);

    return BuildPacket(MessageType::ABORT_VOTE_CAST, requestId, payload);
}

Packet PacketCodec::Encode(const AbortVoteResult& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.partyId, message.aborted, message.stopVotes, message.totalVoters);

    return BuildPacket(MessageType::ABORT_VOTE_RESULT, requestId, payload);
}

// 좌표 테스트 =======================================================================

Packet PacketCodec::Encode(const MarkerRequest& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.x, message.y);

    return BuildPacket(MessageType::MARKER_REQUEST, requestId, payload);
}

Packet PacketCodec::Encode(const MarkerPlaced& message, uint32_t requestId)
{
    Packet payload;
    WriteFields(payload, message.senderId, message.x, message.y);

    return BuildPacket(MessageType::MARKER_PLACED, requestId, payload);
}

// DECODE 부분 

namespace {

    class PacketReader {
    public:
        explicit PacketReader(const PacketCodec::Packet& packet) : packet_(packet) {}

        // 아직 읽지 않은 바이트 수
        size_t Remaining() const
        {
            return packet_.size() - position_;
        }

        // uint8_t, uint16_t, uint32_t를 읽는다.
        template <typename UInt>
        bool Read(UInt& out)
        {
            if (Remaining() < sizeof(UInt)) { return false; }

            uint32_t value = 0;

            for (size_t i = 0; i < sizeof(UInt); ++i) {
                value = (value << 8) | packet_[position_++];
            }

            out = static_cast<UInt>(value);
            return true;
        }

        // 여러 정수 필드를 나열한 순서대로 읽는다.
        template <typename... Fields>
        bool ReadFields(Fields&... fields)
        {
            return (Read(fields) && ...);
        }

        // [문자열 길이 2바이트][문자열 바이트]
        bool ReadString(uint16_t& length, string& text)
        {
            if (!Read(length)) {
                return false;
            }

            if (Remaining() < length) {
                return false;
            }

            text.clear();

            for (size_t i = 0; i < length; ++i) {
                text.push_back(static_cast<char>(packet_[position_ + i]));
            }

            position_ += length;
            return true;
        }

    private:
        const PacketCodec::Packet& packet_;
        size_t position_ = 0;
    };

    // 공통 구조체

    bool ReadRecord(PacketReader& reader, PartyMemberInfo& member)
    {
        return reader.Read(member.playerId) && reader.ReadString(member.nmaeLength, member.name);
    }

    bool ReadRecord(PacketReader& reader, PlayerInitInfo& player)
    {
        return reader.ReadFields(player.playerId, player.mana, player.maxMana);
    }

    bool ReadRecord(PacketReader& reader, StructureInitInfo& structure)
    {
        return reader.ReadFields(structure.structureId, structure.type, structure.x, structure.y, structure.hp, structure.maxHp);
    }

    bool ReadRecord(PacketReader& reader, UnitStateInfo& unit)
    {
        return reader.ReadFields(unit.unitId, unit.ownerId, unit.unitType, unit.x, unit.y, unit.hp, unit.maxHp, unit.action);
    }

    bool ReadRecord(PacketReader& reader, StructureStateInfo& structure)
    {
        return reader.ReadFields(structure.structureId, structure.hp, structure.maxHp);
    }

    // [요소 개수][각 요소의 데이터]
    template <typename Count, typename Item>
    bool ReadList(PacketReader& reader, Count& count, vector<Item>& items)
    {
        if (!reader.Read(count)) {
            return false;
        }

        items.clear();

        for (size_t i = 0; i < count; ++i) {
            Item item{};

            if (!ReadRecord(reader, item)) {
                return false;
            }

            items.push_back(move(item));
        }

        return true;
    }

    // 역직렬화

    // 접속 및 맵 참가 =============================================================

    bool Deserialize(PacketReader& reader, Hello& message)
    {
        return reader.ReadString(message.nmaeLength, message.name);
    }

    bool Deserialize(PacketReader& reader, Welcome& message)
    {
        return reader.Read(message.playerId);
    }

    bool Deserialize(PacketReader& reader, JoinMap& message)
    {
        return reader.Read(message.mapId);
    }

    // 파티 =======================================================================

    bool Deserialize(PacketReader& reader, PartyState& message)
    {
        return reader.ReadFields(message.partyId, message.mapId, message.leaderId) && ReadList(reader, message.memberCount, message.members);
    }

    bool Deserialize(PacketReader& reader, StartGame& message)
    {
        return reader.Read(message.partyId);
    }

    bool Deserialize(PacketReader& reader, LeaveParty& message)
    {
        return reader.Read(message.partyId);
    }

    bool Deserialize(PacketReader& reader, LeaveApproved& message)
    {
        return reader.Read(message.partyId);
    }

    // 채팅 =======================================================================

    bool Deserialize(PacketReader& reader, ChatSend& message)
    {
        return reader.ReadString(message.textLength, message.text);
    }

    bool Deserialize(PacketReader& reader, ChatMessage& message)
    {
        return reader.Read(message.senderId)
            && reader.ReadString(message.textLength, message.text);
    }

    // 전투 초기화 및 소환 =======================================================================

    bool Deserialize(PacketReader& reader, BattleInit& message)
    {
        return reader.ReadFields(message.partyId, message.mapId, message.remainingSeconds)
            && ReadList(reader, message.playerCount, message.players)
            && ReadList(reader, message.structureCount, message.structures);
    }

    bool Deserialize(PacketReader& reader, SpawnRequest& message)
    {
        return reader.ReadFields(message.unitType, message.x, message.y);
    }

    bool Deserialize(PacketReader& reader, UnitCreated& message)
    {
        return reader.ReadFields(message.unitId, message.ownerId, message.unitType, message.x, message.y, message.hp, message.maxHp);
    }

    bool Deserialize(PacketReader& reader, ManaState& message)
    {
        return reader.ReadFields(message.mana, message.maxMana);
    }

    bool Deserialize(PacketReader& reader, RequestRejected& message)
    {
        return reader.ReadFields(message.rejectedType, message.errorCode);
    }

    // 전투 진행 및 결과 =======================================================================

    bool Deserialize(PacketReader& reader, BattleState& message)
    {
        return reader.ReadFields(message.partyId, message.serverTick)
            && ReadList(reader, message.unitCount, message.units)
            && ReadList(reader, message.structureCount, message.structures);
    }

    bool Deserialize(PacketReader& reader, BattleProgress& message)
    {
        return reader.ReadFields(message.partyId, message.remainingSeconds, message.destructionPercent, message.stars);
    }

    bool Deserialize(PacketReader& reader, BattleResult& message)
    {
        return reader.ReadFields(message.partyId, message.reason, message.destructionPercent, message.stars, message.returnSeconds);
    }

    bool Deserialize(PacketReader& reader, ReturnLobby& message)
    {
        return reader.Read(message.partyId);
    }

    // 중단 투표 =======================================================================

    bool Deserialize(PacketReader& reader, AbortVoteStart& message)
    {
        return reader.ReadFields(message.partyId, message.leaverId, message.voteSeconds, message.requestedVotes, message.totalVoters);
    }

    bool Deserialize(PacketReader& reader, AbortVoteCast& message)
    {
        return reader.ReadFields(message.partyId, message.stop);
    }

    bool Deserialize(PacketReader& reader, AbortVoteResult& message)
    {
        return reader.ReadFields(message.partyId, message.aborted, message.stopVotes, message.totalVoters);
    }

    // 좌표 테스트 =======================================================================

    bool Deserialize(PacketReader& reader, MarkerRequest& message)
    {
        return reader.ReadFields(message.x, message.y);
    }

    bool Deserialize(PacketReader& reader, MarkerPlaced& message)
    {
        return reader.ReadFields(message.senderId, message.x, message.y);
    }

    // 모든 Decode가 공유하는 헤더 검사와 본문 복원

    template <typename Body>
    bool DecodePacket(const Packet& packet, MessageType expectedType, Body& out, PacketHeader* header)
    {
        PacketReader reader(packet);
        PacketHeader receivedHeader{};

        // 헤더 10바이트 읽기
        if (!reader.ReadFields(
            receivedHeader.totalLength,
            receivedHeader.messageType,
            receivedHeader.requestId)) {
            return false;
        }

        // 헤더에 적힌 길이와 실제 패킷 길이 비교
        if (receivedHeader.totalLength != packet.size()) {
            return false;
        }

        // 복원하려는 구조체와 메시지 번호가 맞는지 확인
        if (receivedHeader.messageType != static_cast<uint16_t>(expectedType)) {
            return false;
        }

        // 임시 구조체에 먼저 복원한다.
        Body decoded{};

        if (!Deserialize(reader, decoded)) {
            return false;
        }

        // 정의된 본문을 다 읽은 후 남는 바이트가 있으면 오류
        if (reader.Remaining() != 0) {
            return false;
        }

        // 모든 검사가 성공한 경우에만 호출자에게 결과 전달
        out = move(decoded);

        if (header != nullptr) {
            *header = receivedHeader;
        }

        return true;
    }

} // namespace


// 접속 및 맵 참가 =======================================================================

bool PacketCodec::Decode(const Packet& packet, Hello& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::HELLO, out, header);
}

bool PacketCodec::Decode(const Packet& packet, Welcome& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::WELCOME, out, header);
}

bool PacketCodec::Decode(const Packet& packet, JoinMap& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::JOIN_MAP, out, header);
}

// 파티 =======================================================================

bool PacketCodec::Decode(const Packet& packet, PartyState& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::PARTY_STATE, out, header);
}

bool PacketCodec::Decode(const Packet& packet, StartGame& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::START_GAME, out, header);
}

bool PacketCodec::Decode(const Packet& packet, LeaveParty& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::LEAVE_PARTY, out, header);
}

bool PacketCodec::Decode(const Packet& packet, LeaveApproved& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::LEAVE_APPROVED, out, header);
}

// 채팅 =======================================================================

bool PacketCodec::Decode(const Packet& packet, ChatSend& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::CHAT_SEND, out, header);
}

bool PacketCodec::Decode(const Packet& packet, ChatMessage& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::CHAT_MESSAGE, out, header);
}

// 전투 초기화 및 소환 =======================================================================

bool PacketCodec::Decode(const Packet& packet, BattleInit& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::BATTLE_INIT, out, header);
}

bool PacketCodec::Decode(const Packet& packet, SpawnRequest& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::SPAWN_REQUEST, out, header);
}

bool PacketCodec::Decode(const Packet& packet, UnitCreated& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::UNIT_CREATED, out, header);
}

bool PacketCodec::Decode(const Packet& packet, ManaState& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::MANA_STATE, out, header);
}

bool PacketCodec::Decode(const Packet& packet, RequestRejected& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::REQUEST_REJECT, out, header);
}

// 전투 진행 및 결과 =======================================================================

bool PacketCodec::Decode(const Packet& packet, BattleState& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::BATTLE_STATE, out, header);
}

bool PacketCodec::Decode(const Packet& packet, BattleProgress& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::BATTLE_PROGRESS, out, header);
}

bool PacketCodec::Decode(const Packet& packet, BattleResult& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::BATTLE_RESULT, out, header);
}

bool PacketCodec::Decode(const Packet& packet, ReturnLobby& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::RETURN_LOBBY, out, header);
}

// 중단 투표 =======================================================================

bool PacketCodec::Decode(const Packet& packet, AbortVoteStart& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::ABORT_VOTE_START, out, header);
}

bool PacketCodec::Decode(const Packet& packet, AbortVoteCast& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::ABORT_VOTE_CAST, out, header);
}

bool PacketCodec::Decode(const Packet& packet, AbortVoteResult& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::ABORT_VOTE_RESULT, out, header);
}

// 좌표 테스트 =======================================================================

bool PacketCodec::Decode(const Packet& packet, MarkerRequest& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::MARKER_REQUEST, out, header);
}

bool PacketCodec::Decode(const Packet& packet, MarkerPlaced& out, PacketHeader* header)
{
    return DecodePacket(packet, MessageType::MARKER_PLACED, out, header);
}