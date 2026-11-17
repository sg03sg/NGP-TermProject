#pragma once

#include <cstdint>
#include <vector>
#include "Unit.h"

using namespace std;

struct BattleUnit {
    uint32_t ownerId;
    Unit unit;
};

class BattleInstance {
public:
    int RegisterUnit(
        uint32_t ownerId, UnitType type, int x, int y
    );

    Unit* SpawnUnit(
        uint32_t ownerId, UnitType type,
        int x, int y, const UnitStats& stats
    );

    Unit* FindUnit(int id);
    int CountUnits(UnitType type) const;

    void ClearUnits();
    void ClearUnits(UnitType type);
    void RemoveDeadEntities();

    vector<BattleUnit>& GetUnits();
    const vector<BattleUnit>& GetUnits() const;

private:
    vector<BattleUnit> units;
    uint32_t nextUnitId = 1;
};