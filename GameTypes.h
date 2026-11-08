#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <windows.h>
// 여러 모듈에서 공통으로 사용하는 게임 자료형과 상수를 정의한다
#define MAX_UNITS 100
#define MAX_TOWER 10
#define MAX_INFERNOS 10
#define IDT_ANIM_TIMER 1
#define IDT_STAGE_TIMER 2
#define IDT_LOADING_TIMER 3
#define MAX_EFFECTS 100
#define INITIAL_EFFECT_CAPACITY 200
#define DOOR_COUNT 3

typedef struct {
    int x, y;
    BOOL active;
    int hp, maxHp;
    int atk;
    int range;
    int attackCooldown;
} tower;

typedef struct {
	int x, y;
	BOOL active;
	int hp, maxHp;
	int attackCooldown;
	float damageRatio;  // 체력 비례 데미지
	BOOL isAttacking;
	int targetId;           // 공격 중인 유닛 id
} InfernoTower;

typedef struct {
    float x, y;               // 현재 위치
    float vx, vy;             // 속도 벡터
    int targetX, targetY;     // 목표 위치 (대포 중심)
    int life;                 // 남은 프레임
    BOOL active;
    int damage;
    int targetIndex;
    int towerindex;
} ArrowEffect;

typedef struct {
    float x, y;               // 현재 위치
    float vx, vy;             // 속도 벡터
    int life;                 // 남은 프레임
    BOOL active;
    int targetId;             // 대상 유닛 id
    int sourceTowerId;
} InfernoEffect;

typedef struct {
    float x, y;
    float vx, vy;
    int targetX, targetY;     // 목표 위치 (대포 중심)
    int targetId;             // 대상 유닛 id
    int life;
    BOOL active;
    int totalLife;         // 포물선 시각 효과 계산용
    BOOL isMortarShot;     // 대포인지 박격포인지 구분 (간단한 BOOL로 처리) 
} shot;

#endif
