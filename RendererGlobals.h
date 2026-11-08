#ifndef CLIENT_GLOBALS_H
#define CLIENT_GLOBALS_H

#include "GameTypes.h"
#include "Unit.h"

// coc.c 가 소유한 전역 상태를 클라이언트 모듈들(Renderer 등)이 참조하기 위한 선언이다.
// 정의는 coc.c 에 있으며, coc.c 도 이 헤더를 포함해 "선언과 정의의 타입 불일치를 컴파일러가 잡게 하기 위함".

typedef enum {
    STATE_LOADING,
    STATE_MENU,
    STATE_STAGE1,
    STATE_STAGE2,
    STATE_STAGE3,
    STATE_SHOP,
    STATE_END
} GameState;

typedef struct UI {
    int x, y;
    int length, height;
}UI;

extern GameState gameState;
extern BOOL isLoading;
extern BOOL showGrid;
extern BOOL showrange;
extern BOOL showblock;
extern BOOL gameend;
extern BOOL victory;
extern int loadingProgress;

extern int tileWidth;
extern int tileHeight;
extern int boardSize;

extern UI uis[4];

extern shot cannonShots[MAX_EFFECTS];
extern int cannonShotCount;
extern ArrowEffect* arrowEffects;
extern int arrowEffectCount;
extern tower cannons[3][MAX_TOWER];
extern int cannonCount[3];
extern tower mortars[3][MAX_TOWER];
extern int mortarCount[3];
extern InfernoTower infernos[3][MAX_TOWER];
extern int infernoCount[3];
extern InfernoEffect infernoEffects[50];
extern int infernoEffectCount;
extern tower villageking;
extern int towercount[3];
extern int naturaltowercount[3];

extern int mana;
extern int earn_star[3];
extern int starCount;
extern TCHAR timeStr[16];

// 메뉴 / 상점 화면
extern int doorFrameIndices[DOOR_COUNT];
extern BOOL barbarianEntering[DOOR_COUNT];
extern RECT doorRects[3];
extern POINT barbarianPos;
extern int barbarianMenuFrame;
extern RECT shopButton;
extern RECT shopunit;
extern RECT btnAtk;
extern RECT btnSpeed;
extern RECT btnRate;
extern int unitAtk;
extern int unitSpeed;
extern int unitRate;

#endif
