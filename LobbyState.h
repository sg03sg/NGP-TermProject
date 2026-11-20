#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include "Protocol.h"

constexpr uint16_t kMsgLobbyState = 24;  // MessageType - LOBBY_STATE 추가
constexpr size_t   kPacketHeaderSize = PACKET_HEADER_SIZE;
constexpr uint8_t  kMaxPartyMembers = 4;
constexpr uint8_t  kStageCount = 3;     // 게임의 스테이지 수
constexpr size_t   kMaxLobbyMaps = 32;  // 디코딩 방어용 상한

enum class MapStatus : uint8_t {
    Open = 0,       // 모집 가능
    Full = 1,       // 정원이 가득 참
    InBattle = 2    // 공략 중 (잠김)
};

struct LobbyMapInfo {
    uint8_t   mapId = 0;
    uint8_t   memberCount = 0;
    uint8_t   capacity = kMaxPartyMembers;
    MapStatus status = MapStatus::Open;
};


inline std::vector<LobbyMapInfo> MakeDefaultLobbyMaps(uint8_t stageCount = kStageCount)
{
    std::vector<LobbyMapInfo> maps;
    for (uint8_t i = 1; i <= stageCount; ++i) {
        LobbyMapInfo m;
        m.mapId = i;
        maps.push_back(m);
    }
    return maps;
}


std::vector<char> EncodeLobbyStateBody(const std::vector<LobbyMapInfo>& maps);
bool DecodeLobbyStateBody(const char* body, size_t length, std::vector<LobbyMapInfo>& out);


std::vector<char> EncodeLobbyStatePacket(const std::vector<LobbyMapInfo>& maps, uint32_t requestId = 0);