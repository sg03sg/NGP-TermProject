#pragma once
#include <map>
#include <vector>
#include "Party.h"
#include "LobbyState.h"    // LobbyMapInfo, kStageCount

// 오류 분류
enum class PartyError : uint16_t {
    None = 0,
    InvalidStage,
    StageLocked,
    PartyFull,
    AlreadyInParty,
    NotInParty,
    NotLeader,
    ChatBlocked    // 도배로 채팅 전송이 막힘
};

// 참가·퇴장 결과
struct PartyResult {
    bool       success = false;
    PartyError error = PartyError::None;
    PartyId    partyId = kInvalidPartyId;
    MapId      mapId = 0;
    bool       createdNew = false;                // JoinMap: 새 파티가 만들어졌다
    bool       leaderChanged = false;             // LeaveParty: 방장이 바뀌었다
    PlayerId   newLeader = kInvalidPlayerId;
    bool       disbanded = false;                 // LeaveParty: 마지막 사람이 나가 파티가 사라졌다
};

class PartyManager {
private:
    std::map<PartyId, Party>   parties_;        // 전체 파티 보관
    std::map<PlayerId, PartyId> playerToParty_; // 플레이어 소속 조회
    std::map<MapId, PartyId>   mapToParty_;     // 맵 점유 현황
    PartyId                    nextPartyId_ = 1;// 파티 ID 발급기
    uint8_t                    stageCount_;

    bool IsValidMap(MapId mapId) const { return mapId >= 1 && mapId <= stageCount_; }
    void EraseParty(PartyId partyId);

public:
    explicit PartyManager(uint8_t stageCount = kStageCount) : stageCount_(stageCount) {}

    PartyResult JoinMap(PlayerId pid, MapId mapId);   // 참가 또는 파티 생성
    bool        IsMapOpen(MapId mapId) const;         // 모집 가능한지 판정
    PartyResult LeaveParty(PlayerId pid);             // 명단에서 제거 (방장 이전, 빈 파티 삭제)
    void        ReleaseMap(MapId mapId);              // 맵 점유 해제 (파티를 해체한다)
    Party* GetParty(PartyId partyId);                 // 게임 스레드 전용 참조
    const Party* GetParty(PartyId partyId) const;
    std::vector<PlayerId> MembersOf(PartyId partyId) const;   // 전송용 참가자 목록

    // 보조 함수
    PartyId PartyOf(PlayerId pid) const;              // 소속 파티 ID, 없으면 kInvalidPartyId
    bool    IsInParty(PlayerId pid) const { return playerToParty_.count(pid) != 0; }
    std::vector<LobbyMapInfo> BuildLobbyMaps() const; // LOBBY_STATE 방송용 현황
};