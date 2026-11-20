#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "LobbyState.h"    // kMaxPartyMembers

using PartyId = uint32_t;
using PlayerId = uint32_t;
using MapId = uint8_t;

constexpr PlayerId kInvalidPlayerId = 0;   // PlayerId는 1부터 발급한다고 가정
constexpr PartyId  kInvalidPartyId = 0;

// 대기·시작·진행 상태
enum class PartyStatus : uint8_t {
    Waiting,    // 모집 중 (파티 대기창)
    Running,    // 전투 진행 중 (스테이지 잠김)
    Finished    // 공략 종료 (곧 해체)
};

struct PartyMemberInfoView {
    PlayerId    playerId = 0;
    std::string name;
};

// BuildPartyState()의 결과
struct PartyInfo {
    PartyId  partyId = 0;
    MapId    mapId = 0;
    PlayerId leaderId = 0;
    std::vector<PartyMemberInfoView> members;
};

using NameLookup = std::function<std::string(PlayerId)>;   // playerId -> 닉네임

class Party {
private:
    PartyId             partyId_;                         // 파티 식별자
    MapId               mapId_;                           // 공략할 맵
    PlayerId            leaderId_ = kInvalidPlayerId;     // 시작 권한 보유자
    std::vector<PlayerId> members_;                       // 참가자 명단 (입장 순서)
    PartyStatus         status_ = PartyStatus::Waiting;   // 대기·시작·진행 상태




    // chatLog_




public:
    Party(PartyId partyId, MapId mapId);

    bool AddMember(PlayerId pid);       // 정원·상태·중복 검사 후 추가. 첫 참가자는 방장이 된다.
    bool RemoveMember(PlayerId pid);    // 임의 위치에서 제거. 방장 이전은 TransferLeader()가 한다.
    bool CanStart(PlayerId pid) const;  // 방장·인원·상태 검사
    PartyInfo BuildPartyState(const NameLookup& nameOf = nullptr) const;  // 명단과 닉네임 복사본
    PlayerId TransferLeader();          // 남은 첫 사람에게 방장 이전. 비었으면 kInvalidPlayerId.
    void Reset();                       // 명단 비우기 (채팅 내역은 추가 시 함께 비운다)

    void SetStatus(PartyStatus s) { status_ = s; }

    PartyId       Id()       const { return partyId_; }
    MapId         GetMapId() const { return mapId_; }
    PlayerId      Leader()   const { return leaderId_; }
    PartyStatus   Status()   const { return status_; }
    const std::vector<PlayerId>& Members() const { return members_; }
    size_t        Size()     const { return members_.size(); }
    bool          IsFull()   const { return members_.size() >= kMaxPartyMembers; }
    bool          IsEmpty()  const { return members_.empty(); }
    bool          Has(PlayerId pid) const;
};