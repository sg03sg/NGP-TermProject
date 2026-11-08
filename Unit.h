#ifndef UNIT_H
#define UNIT_H

#include <vector>
#include "GameTypes.h"

// 유닛. 컨테이너 안에서 이 순서대로 묶여 저장된다
enum class UnitType {
    Barbarian,
    Archer,
    Giant
};

// 유닛별 기본 수치
struct UnitStats {
    int maxHp;
    int atk;
    int range;
    int speed;
    bool isRanged;
};

const UnitStats& GetUnitStats(UnitType type);

class Unit {
public:
    Unit(int id, UnitType type, int x, int y, const UnitStats& stats);

    int GetId() const { return id_; }
    UnitType GetType() const { return type_; }
    int GetHp() const { return hp_; }
    int GetMaxHp() const { return maxHp_; }
    bool IsDead() const { return hp_ <= 0; }
    bool IsRanged() const { return isRanged_; }

    void TakeDamage(int damage);
    void SetTarget(int targetX, int targetY, int towerIndex);
    void AdvanceFrame() { frameIndex = (frameIndex + 1) % 3; }

    int x, y;
    int frameIndex;
    int speed;
    int atk;
    int range;
    int targetX, targetY;   // 목표 위치
    BOOL hasTarget;
    int targetIndex;        // 목표 타워 번호
    int towerindex;
    int attackCooldown;

private:
    int id_;
    UnitType type_;
    int hp_;
    int maxHp_;
    bool isRanged_;
};

// 전체 유닛. 유닛 순서(바바리안, 아처, 자이언트)로 묶이고 유닛 안에서는 소환 순서를 유지한다
extern std::vector<Unit> units;

// 유닛당 MAX_UNITS 를 넘으면 nullptr. id 는 소환할 때 한 번 부여하고 다시 쓰지 않는다
Unit* SpawnUnit(UnitType type, int x, int y, const UnitStats& stats);
Unit* FindUnit(int id);
int CountUnits(UnitType type);
void ClearUnits();
void ClearUnits(UnitType type);

#endif
