#include "BattleInstance.h"
#include <algorithm>
#include <climits>

int BattleInstance::RegisterUnit(
    uint32_t ownerId, UnitType type, int x, int y)
{
    if (type < UnitType::Barbarian || type > UnitType::Giant) {
        return 0;
    }

    Unit* unit = SpawnUnit(ownerId, type, x, y, GetUnitStats(type));

    if (unit == nullptr) {
        return 0;
    }

    return unit->GetId();
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

vector<BattleUnit>& BattleInstance::GetUnits()
{
    return units;
}

const vector<BattleUnit>& BattleInstance::GetUnits() const
{
    return units;
}