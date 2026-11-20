#include "LobbyState.h"

namespace {
    void PutU16(std::vector<char>& buf, uint16_t v) {
        buf.push_back(static_cast<char>((v >> 8) & 0xFF));
        buf.push_back(static_cast<char>(v & 0xFF));
    }
    void PutU32(std::vector<char>& buf, uint32_t v) {
        buf.push_back(static_cast<char>((v >> 24) & 0xFF));
        buf.push_back(static_cast<char>((v >> 16) & 0xFF));
        buf.push_back(static_cast<char>((v >> 8) & 0xFF));
        buf.push_back(static_cast<char>(v & 0xFF));
    }
}

std::vector<char> EncodeLobbyStateBody(const std::vector<LobbyMapInfo>& maps)
{
    const size_t count = maps.size() > kMaxLobbyMaps ? kMaxLobbyMaps : maps.size();

    std::vector<char> body;
    body.reserve(1 + count * 4);
    body.push_back(static_cast<char>(count));
    for (size_t i = 0; i < count; ++i) {
        body.push_back(static_cast<char>(maps[i].mapId));
        body.push_back(static_cast<char>(maps[i].memberCount));
        body.push_back(static_cast<char>(maps[i].capacity));
        body.push_back(static_cast<char>(maps[i].status));
    }
    return body;
}

bool DecodeLobbyStateBody(const char* body, size_t length, std::vector<LobbyMapInfo>& out)
{
    out.clear();
    if (body == nullptr || length < 1)
        return false;

    const size_t count = static_cast<uint8_t>(body[0]);
    if (count > kMaxLobbyMaps || length != 1 + count * 4)
        return false;

    for (size_t i = 0; i < count; ++i) {
        const char* p = body + 1 + i * 4;
        LobbyMapInfo m;
        m.mapId = static_cast<uint8_t>(p[0]);
        m.memberCount = static_cast<uint8_t>(p[1]);
        m.capacity = static_cast<uint8_t>(p[2]);
        const uint8_t status = static_cast<uint8_t>(p[3]);
        if (status > static_cast<uint8_t>(MapStatus::InBattle) || m.memberCount > m.capacity) {
            out.clear();
            return false;
        }
        m.status = static_cast<MapStatus>(status);
        out.push_back(m);
    }
    return true;
}

std::vector<char> EncodeLobbyStatePacket(const std::vector<LobbyMapInfo>& maps, uint32_t requestId)
{
    const std::vector<char> body = EncodeLobbyStateBody(maps);

    std::vector<char> packet;
    packet.reserve(kPacketHeaderSize + body.size());
    PutU32(packet, static_cast<uint32_t>(kPacketHeaderSize + body.size()));  // totalLength
    PutU16(packet, kMsgLobbyState);                                          // messageType
    PutU32(packet, requestId);                                               // requestId
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}