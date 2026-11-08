#include "Unit.h"
#include <algorithm>

std::vector<Unit> units;

namespace {
    int nextUnitId = 1;

    // 바바리안, 아처, 자이언트 순서
    const UnitStats STATS[] = {
        { 100, 10, 25, 7, false },
        { 80, 15, 75, 7, true },
        { 500, 25, 25, 4, false },
    };
}

const UnitStats& GetUnitStats(UnitType type) {
    return STATS[(int)type];
}

Unit::Unit(int id, UnitType type, int x, int y, const UnitStats& stats)
    : x(x), y(y), frameIndex(0), speed(stats.speed), atk(stats.atk), range(stats.range),
    targetX(0), targetY(0), hasTarget(FALSE), targetIndex(-1), towerindex(0), attackCooldown(0),
    id_(id), type_(type), hp_(stats.maxHp), maxHp_(stats.maxHp), isRanged_(stats.isRanged) {
}

void Unit::TakeDamage(int damage) {
    hp_ -= damage;
    if (hp_ < 0) hp_ = 0;
}

void Unit::SetTarget(int tx, int ty, int towerIndex) {
    targetX = tx;
    targetY = ty;
    hasTarget = (tx != -1);
    targetIndex = towerIndex;
}

Unit* SpawnUnit(UnitType type, int x, int y, const UnitStats& stats) {
    if (CountUnits(type) >= MAX_UNITS) return nullptr;

    // 같은 유닛의 맨 뒤에 넣어 유닛 순서를 유지한다
    std::vector<Unit>::iterator pos = units.begin();
    while (pos != units.end() && pos->GetType() <= type)
        ++pos;

    pos = units.insert(pos, Unit(nextUnitId++, type, x, y, stats));
    return &*pos;
}

Unit* FindUnit(int id) {
    for (Unit& u : units) {
        if (u.GetId() == id) return &u;
    }
    return nullptr;
}

int CountUnits(UnitType type) {
    int count = 0;
    for (const Unit& u : units) {
        if (u.GetType() == type) count++;
    }
    return count;
}

void ClearUnits() {
    units.clear();
}

void ClearUnits(UnitType type) {
    units.erase(std::remove_if(units.begin(), units.end(),
        [type](const Unit& u) { return u.GetType() == type; }), units.end());
}
