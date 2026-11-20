#include "BattleInstance.h"
#include <algorithm>
#include <climits>
#include <iostream>

BattleInstance::BattleInstance(PartyId partyId) : partyId_(partyId)
{

}


bool BattleInstance::Initialize(const MapData& mapData, const std::vector<PlayerId>& members)
{
    if (members.empty()) {
        std::cerr << "[BattleInstance] Initialize failed: no members (party " << partyId_ << ")\n";
        return false;
    }

    mapDataId_ = mapData.id;
    players_.clear();
    ClearUnits();
    towers_.clear();
    vote_ = AbortVote{};
    result_ = BattleResult{};
    tick_ = 0;
    remainingMs_ = kBattleTimeLimitMs;

    // 플레이어별 개인 마나
    players_.reserve(members.size());
    for (PlayerId pid : members) {
        PlayerState p;
        p.id = pid;
        p.maxMana = kMaxMana;
        p.mana = kInitialMana;
        players_.push_back(p);
    }

    // 맵 데이터 사본 복사
    towers_ = mapData.towers;
    for (Tower& t : towers_) {
        t.hp = t.maxHp;
        t.destroyed = false;
    }

    townHall_ = mapData.townHall;
    townHall_.hp = townHall_.maxHp;
    townHall_.destroyed = false;

    if (townHall_.maxHp == 0) {
        std::cerr << "[BattleInstance] Initialize failed: map " << int(mapData.id)
            << " has no valid town hall\n";
        return false;
    }
    return true;
}

Result BattleInstance::TrySpawnUnit(
    PlayerId pid, UnitType t, Position pos)
{
    Result result;

    // 1. 종료된 전투에서는 소환할 수 없다.
    if (result_.finished) {
        result.error = ErrorCode::NotInBattle;
        return result;
    }

    // 2. 이 전투에 참가한 플레이어인지 확인한다.
    auto player = std::find_if(
        players_.begin(),
        players_.end(),
        [pid](const PlayerState& state) {
            return state.id == pid;
        }
    );

    if (player == players_.end()) {
        result.error = ErrorCode::NotInBattle;
        return result;
    }

    // 3. 사용할 수 있는 병종인지 확인한다.
    if (t < UnitType::Barbarian || t > UnitType::Giant) {
        result.error = ErrorCode::InvalidUnit;
        return result;
    }

    // 4. 음수 좌표를 거부한다.
    // 맵 경계와 구조물 겹침 검사는 맵 기준을 맞춘 뒤 추가한다.
    if (pos.x < 0 || pos.y < 0) {
        result.error = ErrorCode::InvalidPosition;
        return result;
    }

    // 5. 병종별 최대 개수와 ID 발급 한도를 확인한다.
    if (CountUnits(t) >= MAX_UNITS || nextUnitId > INT_MAX) {
        result.error = ErrorCode::UnitLimit;
        return result;
    }

    // 6. 서버가 병종의 기본 능력치와 마나 비용을 결정한다.
    const UnitStats& stats = GetUnitStats(t);

    int manaCost = 5;

    if (t == UnitType::Giant) {
        manaCost = 10;
    }

    if (player->mana < manaCost) {
        result.error = ErrorCode::NotEnoughMana;
        return result;
    }

    // 7. 새 병력을 만든다.
    const int unitId = static_cast<int>(nextUnitId);

    BattleUnit entry{
        pid,
        Unit(unitId, t, pos.x, pos.y, stats)
    };

    // 기존 병종별 정렬과 같은 병종 내 소환 순서를 유지한다.
    auto position = units.begin();

    while (position != units.end() &&
           position->unit.GetType() <= t) {
        ++position;
    }

    // 목록에 등록한다. 등록 성공 후 ID와 마나를 변경한다.
    units.insert(position, entry);

    ++nextUnitId;
    player->mana -= manaCost;

    // 8. 생성 결과를 반환한다.
    result.success = true;
    result.error = ErrorCode::None;
    result.unitId = static_cast<uint32_t>(unitId);

    return result;
}

// 기존 Unit.cpp에 있던 SpawnUnit,FindUnit,CountUnits,ClearUnits를 이동 완료

Unit* BattleInstance::SpawnUnit(
    uint32_t ownerId, UnitType type,
    int x, int y, const UnitStats& stats)
{
    if (ownerId == 0 ||
        type < UnitType::Barbarian ||
        type > UnitType::Giant ||
        nextUnitId > INT_MAX) {
        return nullptr;
    }

    if (CountUnits(type) >= MAX_UNITS) {
        return nullptr;
    }

    BattleUnit entry{
        ownerId,
        Unit(static_cast<int>(nextUnitId), type, x, y, stats)
    };

    auto position = units.begin();

    while (position != units.end() &&
        position->unit.GetType() <= type) {
        ++position;
    }

    position = units.insert(position, entry);
    ++nextUnitId;

    return &position->unit;
}

Unit* BattleInstance::FindUnit(int id)
{
    for (BattleUnit& entry : units) {
        if (entry.unit.GetId() == id) {
            return &entry.unit;
        }
    }

    return nullptr;
}

int BattleInstance::CountUnits(UnitType type) const
{
    int count = 0;

    for (const BattleUnit& entry : units) {
        if (entry.unit.GetType() == type) {
            ++count;
        }
    }

    return count;
}

void BattleInstance::ClearUnits()
{
    units.clear();
}

void BattleInstance::ClearUnits(UnitType type)
{
    units.erase(
        remove_if(units.begin(), units.end(),
            [type](const BattleUnit& entry) {
                return entry.unit.GetType() == type;
            }),
        units.end()
    );
}

void BattleInstance::RemoveDeadEntities()
{
    units.erase(
        remove_if(units.begin(), units.end(),
            [](const BattleUnit& entry) {
                return entry.unit.IsDead();
            }),
        units.end()
    );
}

std::vector<BattleUnit>& BattleInstance::GetUnits()
{
    return units;
}

const std::vector<BattleUnit>& BattleInstance::GetUnits() const
{
    return units;
}
