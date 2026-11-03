#ifndef BATTLE_LOGIC_H
#define BATTLE_LOGIC_H

#include "GameTypes.h"

void UpdateUnit(Unit* u, tower* cannons, tower* mortars, InfernoTower* infernos,
    int infernoCount, int mortarCount, int cannonCount, BOOL isRanged, int stageIdx);

void InfernoAttack(int stageIdx);

void UpdateTowerCooldowns(int stageIdx);

void RemoveDeadUnits(void);

void UpdateUnits(int stageIdx);

void UpdateArrows(int stageIdx);

void UpdateCannonAttacks(int stageIdx);

void UpdateCannonShots(void);

void UpdateMortarAttacks(int stageIdx);

void UpdateMortarShots(void);

void UpdateBattleTime(void);

void InitializeBattle(void);

void CountStars(int stageIdx);

BOOL CheckStageEnd(int stageIdx);

#endif
