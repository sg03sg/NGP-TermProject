#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <deque>
#include <map>
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

// 채팅 본문 최대 길이와 파티가 보관하는 내역 수
constexpr size_t kMaxChatBytes = 100;     // 본문 최대 길이, 패킷에 실리는 UTF-8 바이트 수
constexpr size_t kMaxChatHistory = 100;   // 파티가 보관하는 채팅 내역 수, 넘으면 오래된 것부터 삭제

// 도배 방지: 판정 시간 안에 한도를 넘겨 보내면 일정 시간 전송을 막음
constexpr size_t kChatBurstLimit = 3;
constexpr int    kChatBurstWindowMs = 3000;
constexpr int    kChatBlockMs = 5000;

using ChatClock = std::chrono::steady_clock;

struct ChatEntry {
    PlayerId    senderId = kInvalidPlayerId;
    std::string text;
};

enum class ChatResult : uint8_t {
    Ok,
    NotMember,     // 이 파티의 참가자가 아님
    InvalidText,   // 비어 있거나 최대 길이를 넘음
    Blocked        // 도배로 전송이 막힌 상태
};
class Party {
private:
    PartyId             partyId_;                         // 파티 식별자
    MapId               mapId_;                           // 공략할 맵
    PlayerId            leaderId_ = kInvalidPlayerId;     // 시작 권한 보유자
    std::vector<PlayerId> members_;                       // 참가자 명단 (입장 순서)
    PartyStatus         status_ = PartyStatus::Waiting;   // 대기·시작·진행 상태




    // 채팅
    std::vector<ChatEntry> chatLog_;                 // 파티 채팅 내역

    struct ChatThrottle {                            // 도배 방지용 보낸 사람별 기록
        std::deque<ChatClock::time_point> recent;    // 판정 시간 안에 보낸 시각
        ChatClock::time_point blockedUntil{};        // 이 시각까지 전송을 막음
    };
    std::map<PlayerId, ChatThrottle> chatThrottle_;



public:
    Party(PartyId partyId, MapId mapId);

    bool AddMember(PlayerId pid);       // 정원·상태·중복 검사 후 추가. 첫 참가자는 방장이 된다.
    bool RemoveMember(PlayerId pid);    // 임의 위치에서 제거. 방장 이전은 TransferLeader()가 한다.
    bool CanStart(PlayerId pid) const;  // 방장·인원·상태 검사
    PartyInfo BuildPartyState(const NameLookup& nameOf = nullptr) const;  // 명단과 닉네임 복사본
    PlayerId TransferLeader();          // 남은 첫 사람에게 방장 이전. 비었으면 kInvalidPlayerId.
    void Reset();                       // 명단과 채팅 내역 비우기
    ChatResult AppendChat(PlayerId pid, const std::string& text, ChatClock::time_point now = ChatClock::now());   // 길이와 도배 검사 후 저장
    const std::vector<ChatEntry>& ChatLog() const { return chatLog_; }
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