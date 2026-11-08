#include "BattleLogic.h"
#include <math.h>
#include <algorithm>
#include <tchar.h>

// 기존 coc.c의 전역 상태를 참조한다. 실제 정의는 coc.c 한 곳에 유지
// 이후 전투 상태를 객체로 묶을 때 이 의존성을 함께 옮긴다.
extern BOOL archerUnlocked;
extern BOOL giantUnlocked;

extern BOOL gameend;
extern BOOL victory;
extern BOOL starEarned[3];
extern int starCount;

extern TCHAR timeStr[16];

extern tower villageking;
extern int tileWidth;
extern int tileHeight;
extern int towercount[3];
extern int unitRate;
extern InfernoEffect infernoEffects[50];
extern int infernoEffectCount;
extern InfernoTower infernos[3][MAX_TOWER];
extern int infernoCount[3];
extern tower cannons[3][MAX_TOWER];
extern int cannonCount[3];
extern tower mortars[3][MAX_TOWER];
extern int mortarCount[3];
extern ArrowEffect* arrowEffects;
extern int arrowEffectCount;
extern shot cannonShots[MAX_EFFECTS];
extern int cannonShotCount;
extern int boardSize;
extern int Stagetime;
extern int mana;
extern int earn_star[3];
extern int naturaltowercount[3];


// 전투 조건(Update에 해당) --------------------------------------------------------------------------------------------------------------------------------------
// 기존 화살 생성 함수를 사용한다. 투사체 갱신은 아직 coc.c에 남아 있다.
void AddArrowEffect(ArrowEffect effect);

void UpdateUnit(Unit* u, tower* cannons, tower* mortars, InfernoTower* infernos, int infernoCount, int mortarCount, int cannonCount, BOOL isRanged, int stageIdx) {
    if (u->IsDead()) return;

    // 타겟 필요성 검사
    BOOL needRetarget = !u->hasTarget || u->targetIndex < 0 || u->targetIndex >= cannonCount ||(u->targetIndex != -1 && (!cannons[u->targetIndex].active || !mortars[u->targetIndex].active || !infernos[u->targetIndex].active));

    // 타겟 재설정
    if (needRetarget) {
        int minDist = 999999;
        BOOL foundTarget = FALSE;

        for (int j = 0; j < cannonCount; j++) {
            if (!cannons[j].active) continue;
            int dx = cannons[j].x - u->x;
            int dy = cannons[j].y - u->y;
            int dist = dx * dx + dy * dy;
            if (dist < minDist) {
                minDist = dist;
                u->targetX = cannons[j].x;
                u->targetY = cannons[j].y;
                u-> targetIndex = j;
                u->towerindex = 1;
                foundTarget = TRUE;
            }
        }

        for (int j = 0; j < mortarCount; j++) {
            if (!mortars[j].active) continue;
            int dx = mortars[j].x - u->x;
            int dy = mortars[j].y - u->y;
            int dist = dx * dx + dy * dy;
            if (dist < minDist) {
                minDist = dist;
                u->targetX = mortars[j].x;
                u->targetY = mortars[j].y;
                u->targetIndex = j;
                u->towerindex = -2;
                foundTarget = TRUE;
            }
        }

        for (int j = 0; j < infernoCount; j++) {
            if (!infernos[j].active) continue;
            int dx = infernos[j].x - u->x;
            int dy = infernos[j].y - u->y;
            int dist = dx * dx + dy * dy;
            if (dist < minDist) {
                minDist = dist;
                u->targetX = infernos[j].x;
                u->targetY = infernos[j].y;
                u->targetIndex = j;
                u->towerindex = -3;
                foundTarget = TRUE;
            }
        }


        if (villageking.active) {
            int dx = villageking.x - u->x;
            int dy = villageking.y - u->y;
            int dist = dx * dx + dy * dy;
            if (dist < minDist) {
                minDist = dist;
                u->targetX = villageking.x;
                u->targetY = villageking.y;
                u->towerindex = -1;
                foundTarget = TRUE;
            }
        }

        if (!foundTarget) return;

        u->hasTarget = TRUE;
    }

    // 이동
    int dx = (u->targetX + 32) - (u->x + tileWidth / 2);
    int dy = (u->targetY + 32) - (u->y + tileHeight / 2);
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > u->range) {
        u->x += (int)(dx / dist * u->speed);
        u->y += (int)(dy / dist * u->speed);
        return; // 아직 도달하지 않았으면 이동만 하고 끝
    }

    // 공격 쿨타임 체크
    if (u->attackCooldown > 0) {
        u->attackCooldown--;
        return;
    }

    // 공격 실행
    if (u->towerindex == -1) {
        // 마을회관 공격
        if (!villageking.active) return;

        int pdx = (u->x + tileWidth / 2) - (villageking.x + 32);
        int pdy = (u->y + tileHeight / 2) - (villageking.y + 32);
        float attackDist = sqrtf(pdx * pdx + pdy * pdy);

        if (attackDist < u->range) {
            if (isRanged) {
                float startX = u->x + tileWidth / 2;
                float startY = u->y + tileHeight / 2;
                float endX = villageking.x + 32;
                float endY = villageking.y + 32;
                float vx = endX - startX;
                float vy = endY - startY;
                float len = sqrtf(vx * vx + vy * vy);
                float speed = 10.0f;
                ArrowEffect effect = {
                    startX, startY, vx / len * speed, vy / len * speed,
                    endX, endY, 60, TRUE, u->atk, 0,-1
                };
                AddArrowEffect(effect);
            }
            else {
                villageking.hp -= u->atk;
                if (villageking.hp <= 0) {
                    villageking.active = FALSE;
                    villageking.hp = 0;
                }
            }
            u->attackCooldown = 10 + unitRate;
        }

    }
    else if (u->towerindex == -2)
    {
        // 박격포 공격
        tower* c = &mortars[u->targetIndex];
        if (!c->active) return;

        int pdx = (u->x + tileWidth / 2) - (c->x + 32);
        int pdy = (u->y + tileHeight / 2) - (c->y + 32);
        float attackDist = sqrtf(pdx * pdx + pdy * pdy);

        if (attackDist < u->range) {
            if (isRanged) {
                float startX = u->x + tileWidth / 2;
                float startY = u->y + tileHeight / 2;
                float endX = c->x + 32;
                float endY = c->y + 32;
                float vx = endX - startX;
                float vy = endY - startY;
                float len = sqrtf(vx * vx + vy * vy);
                float speed = 10.0f;
                ArrowEffect effect = {
                    startX, startY, vx / len * speed, vy / len * speed,
                    endX, endY, 60, TRUE, u->atk, u->targetIndex,-2
                };
                AddArrowEffect(effect);
            }
            else {
                c->hp -= u->atk;
                if (c->hp <= 0) {
                    c->active = FALSE;
                    c->hp = 0;
                    for (int i = 0; i < 3; i++) towercount[i]--;
                }
            }
            u->attackCooldown = 10 + unitRate;
        }
    }

    else if (u->towerindex == -3)
    {
        // 인페 공격
        //tower* c = &infernos[u->targetIndex];
        InfernoTower* c = &infernos[u->targetIndex];
        if (!c->active) return;

        int pdx = (u->x + tileWidth / 2) - (c->x + 32);
        int pdy = (u->y + tileHeight / 2) - (c->y + 32);
        float attackDist = sqrtf(pdx * pdx + pdy * pdy);

        if (attackDist < u->range) {
            if (isRanged) {
                float startX = u->x + tileWidth / 2;
                float startY = u->y + tileHeight / 2;
                float endX = c->x + 32;
                float endY = c->y + 32;
                float vx = endX - startX;
                float vy = endY - startY;
                float len = sqrtf(vx * vx + vy * vy);
                float speed = 10.0f;
                ArrowEffect effect = {
                    startX, startY, vx / len * speed, vy / len * speed,
                    endX, endY, 60, TRUE, u->atk, u->targetIndex,-3
                };
                AddArrowEffect(effect);
            }
            else {
                c->hp -= u->atk;
                if (c->hp <= 0) {
                    c->active = FALSE;
                    c->hp = 0;
                    for (int i = 0; i < 3; i++) towercount[i]--;
                }
            }
            u->attackCooldown = 10 + unitRate;
        }
    }
    else {
        // 대포 공격
        tower* c = &cannons[u->targetIndex];
        if (!c->active) return;

        int pdx = (u->x + tileWidth / 2) - (c->x + 32);
        int pdy = (u->y + tileHeight / 2) - (c->y + 32);
        float attackDist = sqrtf(pdx * pdx + pdy * pdy);

        if (attackDist < u->range) {
            if (isRanged) {
                float startX = u->x + tileWidth / 2;
                float startY = u->y + tileHeight / 2;
                float endX = c->x + 32;
                float endY = c->y + 32;
                float vx = endX - startX;
                float vy = endY - startY;
                float len = sqrtf(vx * vx + vy * vy);
                float speed = 10.0f;
                ArrowEffect effect = {
                    startX, startY, vx / len * speed, vy / len * speed,
                    endX, endY, 60, TRUE, u->atk, u->targetIndex,0
                };
                AddArrowEffect(effect);
            }
            else {
                c->hp -= u->atk;
                if (c->hp <= 0) {
                    c->active = FALSE;
                    c->hp = 0;
                    for (int i = 0; i < 3; i++) towercount[i]--;
                }
            }
            u->attackCooldown = 10 + unitRate;
        }
    }
}

void InfernoAttack(int stageIdx) {
    BOOL infernoTargetUsed[MAX_UNITS * 3] = { FALSE };

    for (int i = 0; i < infernoCount[stageIdx]; i++) {
        InfernoTower* t = &infernos[stageIdx][i];
        if (!t->active) continue;

        if (t->attackCooldown > 0) {
            t->attackCooldown--;
            continue;
        }

        BOOL valid = FALSE;
        int tx = 0, ty = 0;

        // 현재 타겟 유효성 검사
        if (t->isAttacking) {
            Unit* target = FindUnit(t->targetId);

            if (target && !target->IsDead()) {
                float dx = (target->x + tileWidth / 2) - (t->x + 32);
                float dy = (target->y + tileHeight / 2) - (t->y + 32);
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < 50) {
                    tx = target->x + tileWidth / 2;
                    ty = target->y + tileHeight / 2;
                    valid = TRUE;
                }
            }
        }

        // 타겟이 무효하면 리셋
        if (!valid) {
            t->isAttacking = FALSE;
            t->targetId = -1;
        }

        // 새 타겟 탐색
        if (!t->isAttacking) {
            float minDist = 999999.0f;
            int bestIdx = -1;

            for (int j = 0; j < (int)units.size(); j++) {
                if (infernoTargetUsed[j]) continue; // 다른 인페르노가 이미 타겟팅한 유닛이면 건너뜀

                Unit* target = &units[j];

                if (target->IsDead()) continue;

                float dx = (target->x + tileWidth / 2) - (t->x + 32);
                float dy = (target->y + tileHeight / 2) - (t->y + 32);
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < 50 && dist < minDist) {
                    minDist = dist;
                    bestIdx = j;
                }
            }

            if (bestIdx != -1) {
                infernoTargetUsed[bestIdx] = TRUE;

                Unit* target = &units[bestIdx];

                t->isAttacking = TRUE;
                t->targetId = target->GetId();

                tx = target->x + tileWidth / 2;
                ty = target->y + tileHeight / 2;

                // 중복 이펙트 방지
                BOOL exists = FALSE;
                for (int k = 0; k < infernoEffectCount; k++) {
                    if (infernoEffects[k].active &&
                        infernoEffects[k].sourceTowerId == i) {
                        exists = TRUE;
                        break;
                    }
                }

                if (!exists && infernoEffectCount < 50) {
                    float sx = t->x + 32;
                    float sy = t->y + 32;
                    float dx = tx - sx;
                    float dy = ty - sy;
                    float len = sqrtf(dx * dx + dy * dy);

                    infernoEffects[infernoEffectCount++] = InfernoEffect{
                        sx, sy, dx / len, dy / len,
                        999, TRUE,
                        t->targetId,
                        i
                    };
                }
                t->attackCooldown = 1;
                valid = TRUE;
            }
        }

        // 데미지 처리
        if (t->isAttacking && valid) {
            Unit* target = FindUnit(t->targetId);

            if (target && !target->IsDead()) {
                int damage = (int)(target->GetMaxHp() * t->damageRatio);
                if (damage < 1) damage = 1;
                target->TakeDamage(damage);
                t->attackCooldown = 1;
            }
        }
    }
}

// 인페르노 효과의 비활성화와 수명 감소를 처리한다. (이전에는 그리기 코드 안에서 처리하던 부분)
void UpdateInfernoEffects(int stageIdx) {
    for (int i = 0; i < infernoEffectCount; i++) {
        if (!infernoEffects[i].active) continue;

        int towerId = infernoEffects[i].sourceTowerId;
        if (!infernos[stageIdx][towerId].isAttacking) {
            infernoEffects[i].active = FALSE;
            continue;
        }

        if (!FindUnit(infernoEffects[i].targetId)) {
            infernoEffects[i].active = FALSE;
            continue;
        }

        infernoEffects[i].life--;
        if (infernoEffects[i].life <= 0) {
            infernoEffects[i] = infernoEffects[--infernoEffectCount];
            i--;
        }
    }
}

void UpdateTowerCooldowns(int stageIdx)
{
    // [0] 타워 공격 쿨다운 감소 - 옮겨야함 
    for (int i = 0; i < cannonCount[stageIdx]; i++) {
        if (cannons[stageIdx][i].attackCooldown > 0)
            cannons[stageIdx][i].attackCooldown--;
    }
}
// RemoveDeadEntities()에 해당 -----------------------------------------------------------------------
void RemoveDeadUnits(void)
{
    units.erase(std::remove_if(units.begin(), units.end(),
        [](const Unit& u) { return u.IsDead(); }), units.end());
}

void UpdateUnits(int stageIdx)
{
    // 이동 공격 처리 옮겨야함
    for (Unit& u : units)
        UpdateUnit(&u, cannons[stageIdx], mortars[stageIdx], infernos[stageIdx], infernoCount[stageIdx], mortarCount[stageIdx], cannonCount[stageIdx], u.IsRanged(), stageIdx);
    // 이동 공격 처리 끝
}

void UpdateArrows(int stageIdx)
{
    // 여기도 옮겨야 함 
            // 화살 이펙트 이동 및 충돌 판정
    for (int i = 0; i < arrowEffectCount; ) {
        ArrowEffect* arrow = &arrowEffects[i];
        if (!arrow->active) {
            i++;
            continue;
        }

        arrow->x += arrow->vx;
        arrow->y += arrow->vy;
        arrow->life--;

        float dx = arrow->x - arrow->targetX;
        float dy = arrow->y - arrow->targetY;
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < 10.0f || arrow->life <= 0) {

            if (arrow->towerindex == -1) {
                // 마을회관을 타겟한 경우
                tower* v = &villageking;
                if (v->active) {
                    v->hp -= arrow->damage;
                    if (v->hp <= 0) {
                        v->hp = 0;
                        v->active = FALSE;
                        // 마을회관 파괴 시 처리 추가 가능
                    }
                }
            }

            else if (arrow->towerindex == -2)
            {
                tower* c = &mortars[stageIdx][arrow->targetIndex];
                if (c->active) {
                    c->hp -= arrow->damage;
                    if (c->hp <= 0) {
                        c->hp = 0;
                        c->active = FALSE;
                        towercount[stageIdx]--;
                    }
                }
            }

            else if (arrow->towerindex == -3)
            {
                //tower* c = &infernos[stageIdx][arrow->targetIndex];
                InfernoTower* c = &infernos[stageIdx][arrow->targetIndex];
                if (c->active) {
                    c->hp -= arrow->damage;
                    if (c->hp <= 0) {
                        c->hp = 0;
                        c->active = FALSE;
                        towercount[stageIdx]--;
                    }
                }
            }

            else
            {
                tower* c = &cannons[stageIdx][arrow->targetIndex];
                if (c->active) {
                    c->hp -= arrow->damage;
                    if (c->hp <= 0) {
                        c->hp = 0;
                        c->active = FALSE;
                        towercount[stageIdx]--;
                    }
                }
            }

            arrowEffects[i] = arrowEffects[arrowEffectCount - 1];
            arrowEffectCount--;
        }
        else {
            i++;
        }
    }
    // 여기까지 
}
void UpdateCannonAttacks(int stageIdx)
{
    // 여기도 옮기기 
            // [8] 대포 -> 범위 내 유닛에게 대포알 발사
    for (int i = 0; i < cannonCount[stageIdx]; i++) {
        tower* c = &cannons[stageIdx][i];
        if (!c->active) continue;

        // 가장 가까운 유닛 탐색
        float minDist = 999999.0f;
        Unit* best = nullptr;

        for (Unit& u : units) {
            float dx = (u.x + tileWidth / 2) - (c->x + tileWidth);
            float dy = (u.y + tileHeight / 2) - (c->y + tileWidth);
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 100 && dist < minDist) {
                minDist = dist;
                best = &u;
            }
        }

        // 유닛 발견 시 대포알 발사
        if (best && c->attackCooldown == 0) {
            shot shot;
            shot.x = c->x + 32;
            shot.y = c->y + 32;
            shot.targetId = best->GetId();
            shot.active = TRUE;
            shot.life = 60;
            shot.isMortarShot = FALSE;

            float tx = best->x + tileWidth / 2;
            float ty = best->y + tileHeight / 2;

            float dx = tx - shot.x;
            float dy = ty - shot.y;
            float len = sqrtf(dx * dx + dy * dy);
            float speed = 7.0f;

            shot.vx = dx / len * speed;
            shot.vy = dy / len * speed;

            if (cannonShotCount < MAX_EFFECTS) {
                cannonShots[cannonShotCount++] = shot;
            }
            c->attackCooldown = 15;
        }
    }
    // 여기까지 
}
void UpdateCannonShots(void)
{
    // 여기도 옮겨야함 
            //대포알
    for (int i = 0; i < cannonShotCount;) {
        shot* p = &cannonShots[i];
        if (!p->active || p->isMortarShot) {
            i++;
            continue;
        }

        p->x += p->vx;
        p->y += p->vy;
        p->life--;

        // 타겟 유닛이 죽었거나 사라졌으면 탄 제거
        Unit* target = FindUnit(p->targetId);
        if (!target || target->IsDead()) {
            cannonShots[i] = cannonShots[--cannonShotCount];
            continue;
        }

        //  현재 위치로 충돌 체크
        float tx = target->x + tileWidth / 2;
        float ty = target->y + tileHeight / 2;

        float dx = p->x - tx;
        float dy = p->y - ty;
        float dist = sqrtf(dx * dx + dy * dy);
        int cannondamage = 50;

        if (dist < 10.0f || p->life <= 0) {
            target->TakeDamage(cannondamage);

            cannonShots[i] = cannonShots[--cannonShotCount];
            continue;
        }

        i++;  // 다음 탄으로
    }
    // 여기까지 
}
void UpdateMortarAttacks(int stageIdx)
{
    // 여기도 옮겨야함 
            //박격포
    for (int i = 0; i < mortarCount[stageIdx]; i++) {
        tower* m = &mortars[stageIdx][i];
        if (!m->active) continue;
        if (m->attackCooldown > 0) {
            m->attackCooldown--;
            continue;
        }

        float maxDist = -1.0f;
        Unit* best = nullptr;
        float tx = 0, ty = 0;

        for (Unit& u : units) {
            if (u.IsDead()) continue;

            float dx = (u.x + tileWidth / 2) - (m->x + tileWidth / 2);
            float dy = (u.y + tileHeight / 2) - (m->y + tileHeight / 2);
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 60 && dist > maxDist && dist <= m->range) {
                maxDist = dist;
                best = &u;
                tx = u.x + tileWidth / 2;
                ty = u.y + tileHeight / 2;
            }
        }

        if (best && cannonShotCount < MAX_EFFECTS) {
            shot s;
            s.x = m->x + tileWidth / 2;
            s.y = m->y + tileHeight / 2;
            s.targetX = (int)tx;
            s.targetY = (int)ty;
            s.vx = (tx - s.x) / 30.0f;
            s.vy = (ty - s.y) / 30.0f;
            s.life = 30;
            s.totalLife = 30;
            s.active = TRUE;
            s.targetId = best->GetId();
            s.isMortarShot = TRUE;

            cannonShots[cannonShotCount++] = s;
            m->attackCooldown = 30;
        }
    }
    // 여기까지 
}
void UpdateMortarShots(void)
{
    // 여기도 옮겨야함 
            //박격포 탄
    for (int i = 0; i < cannonShotCount;) {
        shot* p = &cannonShots[i];
        if (!p->active || !p->isMortarShot) {
            i++;
            continue;
        }

        // 포탄 이동
        p->x += p->vx;
        p->y += p->vy;
        p->life--;

        if (p->life <= 0) {
            float tx = p->targetX;
            float ty = p->targetY;
            int mortardamage = 80;

            // 범위 안의 유닛 전체 검사
            for (Unit& u : units) {
                float ux = u.x + tileWidth / 2;
                float uy = u.y + tileHeight / 2;
                float d = sqrtf((ux - tx) * (ux - tx) + (uy - ty) * (uy - ty));
                if (d <= 30) {
                    u.TakeDamage(mortardamage);
                }
            }

            cannonShots[i] = cannonShots[--cannonShotCount];
            continue;
        }

        i++;
    }
}
//---------------------------------------------------------------------------------------


// 시간 감소  ------------------------------------------------------------------
void UpdateBattleTime(void)
{
    Stagetime--;
   /* if (Stagetime > 0) {
        Stagetime--;
    }*/
}
// --------------------------------------------------------------------------------------

// 마나, 타이머 초기화(Initialize에 해당) ----------------------------------------------------------
void InitializeBattle(void)
{
    int earn = earn_star[0] + earn_star[1] + earn_star[2];

    if (earn < 3)
        mana = 100;
    else if (earn < 6)
        mana = 110;
    else
        mana = 120;

    Stagetime = 180;
}

// 종료 조건 (CheckResult에 해당)--------------------------------------------------------------
BOOL CheckStageEnd(int stageIdx)
{
    int starPlus = 0;

    if (!villageking.active && !gameend)
    {
        gameend = TRUE;
        victory = TRUE;

        if (!starEarned[stageIdx]) {
            if (!villageking.active) starPlus = 3;
            else if (towercount[stageIdx] >= naturaltowercount[stageIdx] * 0.7f) starPlus = 2;
            else if (towercount[stageIdx] >= naturaltowercount[stageIdx] * 0.4f) starPlus = 1;


            if (starPlus > earn_star[stageIdx]) {
                int delta = starPlus - earn_star[stageIdx];
                earn_star[stageIdx] = starPlus;
                starCount += delta;

                if (earn_star[stageIdx] == 3)
                    starEarned[stageIdx] = TRUE;
            }
        }
    }
    // 시간판정 
    else if (_tcscmp(timeStr, TEXT("00:00")) == 0)
    {
        gameend = TRUE;
        victory = FALSE;

        if (starEarned[stageIdx]) return TRUE;

        if (!villageking.active) starPlus = 3;
        else if (towercount[stageIdx] <= naturaltowercount[stageIdx] * 0.4f) starPlus = 2;
        else if (towercount[stageIdx] <= naturaltowercount[stageIdx] * 0.7f) starPlus = 1;


        if (starPlus > earn_star[stageIdx]) {
            int delta = starPlus - earn_star[stageIdx];
            earn_star[stageIdx] = starPlus;
            starCount += delta;

            if (earn_star[stageIdx] == 3)
                starEarned[stageIdx] = TRUE;
        }

    }

    int star = earn_star[0] + earn_star[1] + earn_star[2];
    if (!archerUnlocked && star >= 3) {
        archerUnlocked = TRUE;
    }
    if (!giantUnlocked && star >= 6) {
        giantUnlocked = TRUE;
    }

    return gameend;
}