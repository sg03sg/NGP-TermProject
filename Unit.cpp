#include "Unit.h"
#include <algorithm>

//std::vector<Unit> units;

namespace {
    //int nextUnitId = 1;

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

// 기존 spawnUnit, FindUnit, CountUnits, ClearUnits를 BattleInstance의 멤버함수로 이동 
