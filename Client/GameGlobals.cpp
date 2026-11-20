#include "../RendererGlobals.h"

// Renderer가 읽는 전역 상태의 정의
// 솔로 게임의 coc.cpp 전역 중 드로잉에 필요한 것만 복사

BattleInstance gameBattle(0);

GameState gameState = STATE_LOADING;

BOOL isLoading = TRUE;
BOOL showGrid = FALSE;
BOOL showrange = FALSE;
BOOL showblock = FALSE;
BOOL gameend = FALSE;
BOOL victory = FALSE;
int loadingProgress = 0;

int tileWidth = 31;
int tileHeight = 22;
int boardSize = 29;

UI uis[4] = {
    { 320, 730, 700, 150 },
    { 390, 750, 70, 100 },
    { 50, 50, 110, 30 },
    { 1060, 50, 50, 50 }
};

shot cannonShots[MAX_EFFECTS];
int cannonShotCount = 0;

ArrowEffect* arrowEffects = NULL;
int arrowEffectCount = 0;

tower cannons[3][MAX_TOWER];
int cannonCount[3] = { 0 };
tower mortars[3][MAX_TOWER];
int mortarCount[3] = { 0 };
InfernoTower infernos[3][MAX_TOWER];
int infernoCount[3] = { 0 };
InfernoEffect infernoEffects[50];
int infernoEffectCount = 0;
tower villageking = { 0 };
int towercount[3] = { 0 };
int naturaltowercount[3] = { 0 };

int mana = 0;
int earn_star[3] = { 0 };
int starCount = 0;
TCHAR timeStr[16] = TEXT("03:00");

// 메뉴 / 상점 화면
int doorFrameIndices[DOOR_COUNT] = { 0, 0, 0 };
BOOL barbarianEntering[DOOR_COUNT] = { FALSE, FALSE, FALSE };
RECT doorRects[3] = {
    { 400, 350, 500, 450 },
    { 600, 350, 700, 450 },
    { 800, 350, 900, 450 }
};
POINT barbarianPos = { 0 };
int barbarianMenuFrame = 0;
RECT shopButton = { 50, 800, 200, 860 };
RECT shopunit = { 0, 270, 300, 400 };
RECT btnAtk = { 540, 700, 740, 740 };
RECT btnSpeed = { 540, 760, 740, 800 };
RECT btnRate = { 540, 820, 740, 860 };
int unitAtk = 0;
int unitSpeed = 0;
int unitRate = 0;
