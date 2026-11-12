#pragma once
#include <vector>
#include <cstdint>

class ClientSession;


using PartyId = uint32_t;
using PlayerId = uint32_t;
using MapDataId = uint8_t;

// ---- 전투 상수 ----
constexpr uint32_t kBattleTimeLimitMs = 3 * 60 * 1000; // 제한 시간 3분
constexpr uint16_t kMaxMana = 100;
constexpr uint16_t kInitialMana = 0;

// ---- 열거형 ----
enum class UnitType : uint8_t { Barbarian = 0, Archer = 1, Giant = 2 };
enum class UnitAction : uint8_t { Move = 0, Attack = 1 };

// 소환 거부 사유 (REQUEST_REJECTED의 errorCode로 매핑)
enum class ErrorCode : uint16_t {
    None = 0,
    NotInBattle,
    InvalidUnit,
    InvalidPosition,
    NotEnoughMana,
    UnitLimit
};

// 중단 투표 집계 결과
enum class VoteOutcome : uint8_t { Pending = 0, Abort, Continue };

// ---- 데이터 구조체 ----
struct Position {
    int32_t x = 0;
    int32_t y = 0;
};

struct PlayerState {            // 개인 마나 등 상태
    PlayerId id = 0;
    uint16_t mana = 0;
    uint16_t maxMana = kMaxMana;
};

struct Unit {                   // units_ : 생성 순서 유지 배열
    uint32_t   id = 0;
    PlayerId   ownerId = 0;
    UnitType   type = UnitType::Barbarian;
    Position   pos;
    uint32_t   hp = 0;
    uint32_t   maxHp = 0;
    UnitAction action = UnitAction::Move;
};

struct Tower {                  // towers_ : 적 방어타워 사본
    uint32_t id = 0;
    Position pos;
    uint32_t hp = 0;
    uint32_t maxHp = 0;
    bool     destroyed = false;
};

struct Structure {              // townHall_ : 핵심 구조물
    uint32_t id = 0;
    Position pos;
    uint32_t hp = 0;
    uint32_t maxHp = 0;
    bool     destroyed = false;
};

struct AbortVote {              // vote_ : 중단 투표 진행 상태
    bool                  active = false;
    PlayerId              leaverId = 0;
    uint32_t              remainingMs = 0;
    std::vector<PlayerId> voters;
    std::vector<PlayerId> stopVotes;
    std::vector<PlayerId> continueVotes;
};

struct BattleResult {           // result_ : 파괴율과 별 개수
    bool    finished = false;
    uint8_t reason = 0;         // 0:진행중 1:전부 파괴 2:시간 종료 3:중단
    uint8_t destructionPct = 0;
    uint8_t stars = 0;
};

struct Result {                 // TrySpawnUnit : 검사·생성·마나 차감 결과
    bool      success = false;
    ErrorCode error = ErrorCode::None;
    uint32_t  unitId = 0;      // 성공 시 생성된 유닛 ID
};

struct Snapshot {               // BuildBattleState / BuildBattleProgress 반환
    
};

// 인원 수에 맞춰 고른 맵 데이터
struct MapData {
    MapDataId          id = 0;
    std::vector<Tower> towers;
    Structure          townHall;
};


class BattleInstance {
private:
    PartyId                  partyId_ = 0;      // 이 전투를 가진 파티
    MapDataId                mapDataId_ = 0;    // 인원에 맞춰 고른 맵
    std::vector<PlayerState> players_;          // 개인 마나 등 상태
    std::vector<Unit>        units_;            // 생성 순서 유지 배열
    std::vector<Tower>       towers_;           // 적 방어타워 사본
    Structure                townHall_;         // 핵심 구조물
    AbortVote                vote_;             // 중단 투표 진행 상태
    uint32_t                 tick_ = 0;         // 경과 틱 수
    uint32_t                 remainingMs_ = 0;  // 남은 제한 시간
    BattleResult             result_;           // 파괴율과 별 개수

public:
    explicit BattleInstance(PartyId partyId);
    ~BattleInstance() = default;

    bool Initialize(const MapData& mapData, const std::vector<PlayerId>& members); // 사본 복사와 초기화
    Result TrySpawnUnit(PlayerId pid, UnitType t, Position pos);                   // 검사·생성·마나 차감
    void Update(uint32_t dtMs);                                                    // 이동·공격·피해 계산
    void RemoveDeadEntities();                                                     // 순서를 지키며 제거
    void CheckResult();                                                            // 종료 조건 판정
    void StartAbortVote(PlayerId leaverId);                                        // 20초 투표 시작
    void CastVote(PlayerId pid, bool stop);                                        // 한 표 기록
    VoteOutcome TallyVote();                                                       // 과반 집계와 확정
    Snapshot BuildBattleState() const;                                             // 병력·구조물 상태 복사
    Snapshot BuildBattleProgress() const;                                          // 남은 시간·파괴율·별
};