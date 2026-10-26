#include <windows.h>
#include <tchar.h>
#include <stdbool.h>
#include <math.h>
#include "resource.h"
#include <mmsystem.h>

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming Lab";

#define MAX_UNITS 100
#define MAX_TOWER 10
#define MAX_INFERNOS 10
#define IDT_ANIM_TIMER 1
#define IDT_STAGE_TIMER 2
#define IDT_LOADING_TIMER 3
#define MAX_EFFECTS 100
#define INITIAL_EFFECT_CAPACITY 200
#define DOOR_COUNT 3


typedef enum {
    STATE_LOADING,
    STATE_MENU,
    STATE_STAGE1,
    STATE_STAGE2,
    STATE_STAGE3,
    STATE_SHOP,
    STATE_END
} GameState;

GameState gameState = STATE_LOADING;
BOOL isLoading = TRUE;
BOOL showGrid = FALSE;
BOOL gameend = FALSE;
BOOL victory = FALSE;
BOOL showrange;
BOOL showblock;
BOOL starEarned[3] = { FALSE, FALSE, FALSE };
BOOL infernoTargetUsed[MAX_UNITS * 3] = { FALSE }; 
BOOL stagestart = FALSE;
BOOL menusoundplay = FALSE;
BOOL stagesoundplay = FALSE;
BOOL unlockOverride = FALSE;
BOOL stage2Unlocked = FALSE;
BOOL stage3Unlocked = FALSE;
BOOL archerUnlocked = FALSE;
BOOL giantUnlocked = FALSE;
int loadingProgress = 0;

int tileWidth = 31;
int tileHeight = 22;
int boardSize = 29;

TCHAR* tips[] = {
    _T("Tip: 전략을 잘 세워 스테이지를 클리어해 보세요."),
    _T("Tip: 클리어가 힘들다면 업그레이드를 활용해 보세요."),
    _T("Tip: 별 3개 도달 시 2스테이지 진입 및 아처 해금입니다."),
	_T("Tip: 별 6개 도달 시 3스테이지 진입 및 자이언트 해금입니다.")
};

typedef struct {
    int x, y;
    int frameIndex;
    int speed;
    POINT dir;
    int hp, maxHp;
    int atk;
    int range;
    int targetX, targetY;  // 목표 위치
    BOOL hasTarget;
    int targetIndex;
    int towerindex;
    int attackCooldown;
} Unit;

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
	int targetIndex;
	BOOL isTargetArcher;
} InfernoTower;

typedef struct {
	float x, y;               // 현재 위치
	float vx, vy;             // 속도 벡터
	int life;                 // 남은 프레임
	BOOL active;
	int targetIndex;
	BOOL isArcher;   
	BOOL isGiant;
	int sourceTowerId;
} InfernoEffect;

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
    float x, y;
    float vx, vy;
    int targetX, targetY;     // 목표 위치 (대포 중심)
    int targetIndex;
    int unittype;
    int life;
    BOOL active;
    int totalLife;         // 포물선 시각 효과 계산용
    BOOL isMortarShot;     // 대포인지 박격포인지 구분 (간단한 BOOL로 처리) 
} shot;

typedef struct UI {
    int x, y;
    int length, height;
}UI;

UI uis[4] = {
    {320,730,700,150},
    {390,750,70,100},
    {50,50,110,30},
    {1060,50,50,50}
};

shot cannonShots[MAX_EFFECTS];
int cannonShotCount = 0;

ArrowEffect* arrowEffects = NULL;
int arrowEffectCount = 0;
int arrowEffectCapacity = 0;

Unit barbarians[MAX_UNITS];
int barbarianCount = 0;

Unit archers[MAX_UNITS];
int archerCount = 0;

Unit giants[MAX_UNITS];
int giantsCount = 0;
int giant_frame = 0;

tower cannons[3][MAX_TOWER];
int cannonCount[3] = { 0 };
int towercount[3] = { 0 };
int naturaltowercount[3] = { 0 };
tower villageking = {0};

tower mortars[3][MAX_TOWER];
int mortarCount[3] = { 0 };

InfernoTower infernos[3][MAX_TOWER];
int infernoCount[3] = { 0 };

InfernoEffect infernoEffects[50];
int infernoEffectCount = 0; 

int mana = 0;
int earn_star[3] = { 0 };

int Stagetime = 0;;
TCHAR timeStr[16];

HBITMAP  hRightWalkBmp[3], hWalkBmp[3],hgiantsbmp[3],hrightgiantbmp[3], hArcherBmp[3],hrightarcherbmp[3], hLogoBmp, hCannonBmp, hmortarbmp, hInfernoBmp, harrowBmp, hcannonBallBmp, hvillagekingBmp, hui0bmp, hui1bmp, hui2bmp, hui3bmp, hui4bmp, hui5bmp, hui6bmp, endvbmp, enddbmp;
BITMAP bmpRightWalk[3], bmpWalk[3],bmpgiants[3],bmprightgiant[3],bmprightarcher[3], aWalk[3], bmpLogo, bmpCannon, bmpmortar, bmpInferno, bmpArrow, bmpCannonBall, bmpvillageking, bmpui0, bmpui1, bmpui2, bmpui3, bmpui4, bmpui5, bmpui6, bmpendv, bmpendd;
HBITMAP hStageMap[3];
BITMAP bmpStageMap[3];
COLORREF baba_back_color;

HBITMAP hDoorFrames[5], hMenuBg, hShopMap,hshopbababmp,hshoparcherbmp,hshopgiantbmp;
BITMAP bmpDoorFrames[5], bmpMenuBg, bmpShopMap, bmpshopbaba,bmpshoparcher,bmpshopgiant;
int doorFrameIndices[DOOR_COUNT] = { 0, 0, 0 };
BOOL doorOpenings[DOOR_COUNT] = { FALSE, FALSE, FALSE };
BOOL barbarianEntering[DOOR_COUNT] = { FALSE, FALSE, FALSE };
int enteringIndex = -1;
RECT doorRects[3] = {
    { 400, 350, 500, 450 },  // 문 1 (64x64)
    { 600, 350, 700, 450 },  // 문 2
    { 800, 350, 900, 450 }   // 문 3
};

int targetStages[DOOR_COUNT] = {
    STATE_STAGE1, STATE_STAGE2, STATE_STAGE3
};
int starCount = 0;
RECT btnAtk = { 540, 700, 740, 740 };
RECT btnSpeed = { 540, 760, 740, 800 };
RECT btnRate = { 540, 820, 740, 860 };
int unitAtk = 0;
int unitSpeed = 0;
int unitRate = 0;
int currentStageIdx = 0;
POINT barbarianPos = { 0 }; // 메뉴 전용 바바리안 위치
int barbarianMenuFrame = 0; // 메뉴 전용 프레임

RECT stageButtons[3];
RECT shopButton;
RECT shopExitButton = { 1160, 210, 1210, 260 };  // 적당한 위치로 수정 가능
RECT shopunit = { 0,270,300,400 };

int upgradelevel[3] = { 0 };

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void InitGame(HWND hWnd) {

    showGrid = FALSE;
    gameend = FALSE;
    victory = FALSE;
    showrange = FALSE;
    showblock = FALSE;

    memset(barbarians, 0, sizeof(barbarians));
    memset(archers, 0, sizeof(archers));
    memset(giants, 0, sizeof(giants));
    memset(naturaltowercount, 0, sizeof(naturaltowercount));

    barbarianCount = archerCount = arrowEffectCount = cannonShotCount= giantsCount = 0;

    int earn = earn_star[0] + earn_star[1] + earn_star[2];
    if (earn < 3)
        mana = 100;
    else if (earn >= 3 && earn < 6)
        mana = 110;
    else if (earn >= 6)
        mana = 120;

    Stagetime = 180;

    lstrcpy(timeStr, TEXT("03:00"));

    cannons[0][0] = (tower){ 625, 380, TRUE, 300, 300, 10 };
    cannons[0][1] = (tower){ 670, 390, TRUE, 300, 300, 10 };
    cannons[0][2] = (tower){ 655, 420, TRUE, 300, 300, 10 };
    cannons[0][3] = (tower){ 610, 410, TRUE, 300, 300, 10 };
    cannons[1][0] = (tower){ 686, 412, TRUE, 300, 300, 10 };
    cannons[1][1] = (tower){ 640, 445, TRUE, 300, 300, 10 }; //한 칸 y =20 x =30
    cannons[1][2] = (tower){ 583, 398, TRUE, 300, 300, 10 };
    cannons[1][3] = (tower){ 645, 366, TRUE, 300, 300, 10 };
    mortars[1][0] = (tower){ 658, 423, TRUE, 300, 300, 10,200 };
    mortars[1][1] = (tower){ 613, 378, TRUE, 300, 300, 10,200 };
    cannons[2][0] = (tower){ 686, 412, TRUE, 300, 300, 10 };
    cannons[2][1] = (tower){ 640, 445, TRUE, 300, 300, 10 };
    cannons[2][2] = (tower){ 583, 398, TRUE, 300, 300, 10 };
    cannons[2][3] = (tower){ 645, 366, TRUE, 300, 300, 10 };
    mortars[2][0] = (tower){ 658, 423, TRUE, 300, 300, 10,500 };
    mortars[2][1] = (tower){ 613, 378, TRUE, 300, 300, 10,500 };
    infernos[2][0] = (InfernoTower){ 659, 378, TRUE, 300, 300, 0, 0.1f, FALSE, -1, FALSE };
    infernos[2][1] = (InfernoTower){ 610, 410, TRUE, 300, 300, 0, 0.1f, FALSE, -1, FALSE };
    villageking = (tower){ 642, 392, TRUE, 600, 600 };

    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        cannonShots[i] = (shot){ 0 };
    }

    cannonCount[0] = 4;
    cannonCount[1] = 4;
    cannonCount[2] = 4;
    mortarCount[1] = 2;
    mortarCount[2] = 2;
    infernoCount[2] = 2;
    towercount[0] = 4;
    towercount[1] = 6; 
    towercount[2] = 8;
    infernoCount[0] = infernoCount[1] = mortarCount[0] = 0;
    for(int i=0; i<3; i++) naturaltowercount[i] = towercount[i];

    hMenuBg = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP24), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hMenuBg) GetObject(hMenuBg, sizeof(BITMAP), &bmpMenuBg);

    for (int i = 0; i < 5; i++) {
        hDoorFrames[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP25 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hDoorFrames[i]) GetObject(hDoorFrames[i], sizeof(BITMAP), &bmpDoorFrames[i]);
    }

    hshoparcherbmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP32), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hshoparcherbmp) GetObject(hshoparcherbmp, sizeof(BITMAP), &bmpshoparcher);

    hshopbababmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP33), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hshopbababmp) GetObject(hshopbababmp, sizeof(BITMAP), &bmpshopbaba);

    hshopgiantbmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP34), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hshopgiantbmp) GetObject(hshopgiantbmp, sizeof(BITMAP), &bmpshopgiant);

    hShopMap = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP26), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hShopMap) GetObject(hShopMap, sizeof(BITMAP), &bmpShopMap);

    endvbmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP22), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (endvbmp) GetObject(endvbmp, sizeof(BITMAP), &bmpendv);

    enddbmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP21), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (enddbmp) GetObject(enddbmp, sizeof(BITMAP), &bmpendd);

    hui0bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP17), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui0bmp) GetObject(hui0bmp, sizeof(BITMAP), &bmpui0);

    hui1bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui1bmp) GetObject(hui1bmp, sizeof(BITMAP), &bmpui1);

    hui2bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP7), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui2bmp) GetObject(hui2bmp, sizeof(BITMAP), &bmpui2);

    hui3bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP16), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui3bmp) GetObject(hui3bmp, sizeof(BITMAP), &bmpui3);

    hui4bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP18), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui4bmp) GetObject(hui4bmp, sizeof(BITMAP), &bmpui4);

    hui5bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP19), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui5bmp) GetObject(hui5bmp, sizeof(BITMAP), &bmpui5);

    hui6bmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP20), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hui6bmp) GetObject(hui6bmp, sizeof(BITMAP), &bmpui6);

    hvillagekingBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP13), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if(hvillagekingBmp) GetObject(hvillagekingBmp, sizeof(BITMAP), &bmpvillageking);

    hLogoBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP41), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hLogoBmp) GetObject(hLogoBmp, sizeof(BITMAP), &bmpLogo);

    for (int i = 0; i < 3; i++) {
        hWalkBmp[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hWalkBmp[i]) GetObject(hWalkBmp[i], sizeof(BITMAP), &bmpWalk[i]);

        hgiantsbmp[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP14 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hgiantsbmp[i]) GetObject(hgiantsbmp[i], sizeof(BITMAP), &bmpgiants[i]);
    }
    for (int i = 0; i < 3; i++) {
        hRightWalkBmp[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP10 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hRightWalkBmp[i]) GetObject(hRightWalkBmp[i], sizeof(BITMAP), &bmpRightWalk[i]);;
    }

    for (int i = 0; i < 3; i++)
    {
        hArcherBmp[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP35 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hArcherBmp[i]) GetObject(hArcherBmp[i], sizeof(BITMAP), &aWalk[i]);

        hrightarcherbmp[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP38 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hrightarcherbmp[i]) GetObject(hrightarcherbmp[i], sizeof(BITMAP), &bmprightarcher[i]);
    }

    harrowBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP8), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (harrowBmp) GetObject(harrowBmp, sizeof(BITMAP), &bmpArrow);

    hCannonBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP5), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hCannonBmp) GetObject(hCannonBmp, sizeof(BITMAP), &bmpCannon);

    hmortarbmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP23), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hmortarbmp) GetObject(hmortarbmp, sizeof(BITMAP), &bmpmortar);

    hInfernoBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP31), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hInfernoBmp) GetObject(hInfernoBmp, sizeof(BITMAP), &bmpInferno);

    hcannonBallBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP9), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    if (hcannonBallBmp) GetObject(hcannonBallBmp, sizeof(BITMAP), &bmpCannonBall);

    for (int i = 0; i < 3; i++) {
        hStageMap[i] = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
        if (hStageMap[i]) GetObject(hStageMap[i], sizeof(BITMAP), &bmpStageMap[i]);
    }

    stagestart = FALSE;

    menusoundplay = FALSE;

    KillTimer(hWnd, IDT_STAGE_TIMER);
    KillTimer(hWnd, IDT_ANIM_TIMER);
    SetTimer(hWnd, IDT_ANIM_TIMER, 200, NULL);
}

void reset_game(HWND hWnd)
{
    InitGame(hWnd);

    for (int i = 0; i < 3; i++)
    {
        starEarned[i] = FALSE;
    }
    starCount = 0;

    unitAtk = 0;
    unitSpeed = 0;
    unitRate = 0;
    archerUnlocked = FALSE;
    giantUnlocked = FALSE;
    for(int i=0; i<3; i++) earn_star[i] = 0;
}

void AddArrowEffect(ArrowEffect effect) {
    if (arrowEffectCount >= arrowEffectCapacity) {
        arrowEffectCapacity += 100;
        arrowEffects = (ArrowEffect*)realloc(arrowEffects, sizeof(ArrowEffect) * arrowEffectCapacity);
    }
    arrowEffects[arrowEffectCount++] = effect;
}

//박격포 탄
void drawShot(HDC hdc, shot* p) {
    float drawX = p->x;
    float drawY = p->y;
	int size = p->isMortarShot ? 24 : 10;
    if (p->isMortarShot) {
        float t = (float)(p->totalLife - p->life) / (float)p->totalLife;
        float height = -50 * t * (1.0f - t); // 포물선 연출
        drawY += height;
    }
	drawX -= size / 2.0f;
	drawY -= size / 2.0f;
    Ellipse(hdc, drawX - 5, drawY - 5, drawX + 5, drawY + 5);
}

void DrawHealthBar(HDC hDC, int x, int y, int width, int height, int hp, int maxHp) {
    int filled = (hp * width) / maxHp;
    RECT bgRect = { x, y - height - 2, x + width, y - 2 };
    RECT fgRect = { x, y - height - 2, x + filled, y - 2 };

    HBRUSH bg = CreateSolidBrush(RGB(60, 60, 60));
    HBRUSH fg = CreateSolidBrush(RGB(0, 255, 0));
    FillRect(hDC, &bgRect, bg);
    FillRect(hDC, &fgRect, fg);
    FrameRect(hDC, &bgRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
    DeleteObject(bg);
    DeleteObject(fg);
}

void DrawLoadingScreen(HDC hDC, HDC mDC, int width, int height) {
    FillRect(mDC, &(RECT){0, 0, width, height}, CreateSolidBrush(RGB(20, 20, 20)));

    if (hLogoBmp) {
        HDC memDC = CreateCompatibleDC(hDC);
        SelectObject(memDC, hLogoBmp);
        StretchBlt(mDC, 0, 0, width, height, memDC, 0, 0, bmpLogo.bmWidth, bmpLogo.bmHeight, SRCCOPY);
        DeleteDC(memDC);
    }

    int bx = (width - 400) / 2;
    int by = height - 145;

    FillRect(mDC, &(RECT){bx, by, bx + 400, by + 30}, CreateSolidBrush(RGB(60, 60, 60)));
    FillRect(mDC, &(RECT){bx + 2, by + 2, bx + 2 + (396 * loadingProgress / 100), by + 28}, CreateSolidBrush(RGB(180, 0, 255)));

    SetBkMode(mDC, TRANSPARENT);
    SetTextColor(mDC, RGB(255, 255, 255));
    DrawText(mDC, tips[loadingProgress % 4], -1, &(RECT){0, by + 40, width, by + 80}, DT_CENTER);
}

void DrawMenuScreen(HDC mDC, int width, int height) {
    if (hMenuBg) {
        HDC memDC = CreateCompatibleDC(mDC);
        SelectObject(memDC, hMenuBg);
        StretchBlt(mDC, 0, 0, width, height, memDC, 0, 0,
            bmpMenuBg.bmWidth, bmpMenuBg.bmHeight, SRCCOPY);
        DeleteDC(memDC);
    }

    // 메뉴 문 출력
    for (int i = 0; i < DOOR_COUNT; i++) {
        HDC doorDC = CreateCompatibleDC(mDC);
        SelectObject(doorDC, hDoorFrames[doorFrameIndices[i]]);
        TransparentBlt(mDC, doorRects[i].left, doorRects[i].top, 100, 100,
            doorDC, 0, 0, bmpDoorFrames[doorFrameIndices[i]].bmWidth, bmpDoorFrames[doorFrameIndices[i]].bmHeight, RGB(255, 255, 255));
        DeleteDC(doorDC);
    }

    // 메뉴 바바리안 출력
    if(barbarianEntering[0] == TRUE || barbarianEntering[1] == TRUE || barbarianEntering[2] == TRUE)
    {
        HDC babaDC = CreateCompatibleDC(mDC);
        SelectObject(babaDC, hRightWalkBmp[barbarianMenuFrame]);
        COLORREF babaBg = GetPixel(babaDC, 0, 1);
        TransparentBlt(mDC, barbarianPos.x, barbarianPos.y, 60, 60,
            babaDC, 0, 0, bmpRightWalk[barbarianMenuFrame].bmWidth, bmpRightWalk[barbarianMenuFrame].bmHeight, babaBg);

        DeleteDC(babaDC);
    }

    // 상점 버튼 출력
    RECT shop = shopButton;
    SetBkMode(mDC, TRANSPARENT);
    SetTextColor(mDC, RGB(255, 255, 255));
    HBRUSH btnBrush = CreateSolidBrush(RGB(100, 60, 150));
    FillRect(mDC, &shop, btnBrush);
    FrameRect(mDC, &shop, (HBRUSH)GetStockObject(WHITE_BRUSH));
    DrawText(mDC, _T("상점"), -1, &shop, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DeleteObject(btnBrush);
	HFONT hFont = CreateFont(
		36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("맑은 고딕")
	);
	HFONT hOldFont = (HFONT)SelectObject(mDC, hFont);
    TCHAR starText[30];
    wsprintf(starText, _T("★ 별 : %d"), starCount);
    SetBkMode(mDC, TRANSPARENT);
    SetTextColor(mDC, RGB(255, 255, 0));
    TextOut(mDC, uis[3].x+40, uis[3].y, starText, lstrlen(starText));
    SelectObject(mDC, hOldFont);
    DeleteObject(hFont);

}

void DrawShopScreen(HDC mDC) {
    if (hShopMap) {
        HDC memDC = CreateCompatibleDC(mDC);
        SelectObject(memDC, hShopMap);
        BitBlt(mDC, 0, 0, bmpShopMap.bmWidth, bmpShopMap.bmHeight, memDC, 0, -200, SRCCOPY);
        DeleteDC(memDC);
    }

    if (hshopbababmp && hshoparcherbmp && hshopgiantbmp) {
        HDC memDC = CreateCompatibleDC(mDC);
        SelectObject(memDC, hshopbababmp);
        COLORREF shopbaba_backcol = GetPixel(memDC, 0, 1);
        TransparentBlt(mDC, shopunit.left+50, shopunit.top, shopunit.right, shopunit.bottom, memDC, 0, 0, bmpshopbaba.bmWidth, bmpshopbaba.bmHeight,shopbaba_backcol);

        SelectObject(memDC, hshoparcherbmp);
        shopbaba_backcol = GetPixel(memDC, 0, 1);
        TransparentBlt(mDC, shopunit.left+ shopunit.right+170, shopunit.top, shopunit.right, shopunit.bottom, memDC, 0, 0, bmpshoparcher.bmWidth, bmpshoparcher.bmHeight, shopbaba_backcol);

        SelectObject(memDC, hshopgiantbmp);
        shopbaba_backcol = GetPixel(memDC, 0, 1);
        TransparentBlt(mDC, shopunit.left + 2*shopunit.right + 320, shopunit.top+30, shopunit.right-20, shopunit.bottom-50, memDC, 0, 0, bmpshopgiant.bmWidth, bmpshopgiant.bmHeight, shopbaba_backcol);

        DeleteDC(memDC);
        
    }
    HFONT hFont = CreateFont(
        36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("맑은 고딕")
    );
    HFONT hOldFont = (HFONT)SelectObject(mDC, hFont);
    TCHAR starText[30];
    wsprintf(starText, _T("★ 별 : %d"), starCount);
    SetBkMode(mDC, TRANSPARENT);
    SetTextColor(mDC, RGB(255, 255, 0));
    TextOut(mDC, uis[3].x + 40, uis[3].y, starText, lstrlen(starText));

    SelectObject(mDC, hOldFont);
    DeleteObject(hFont);

	SetTextColor(mDC, RGB(255, 0, 0));
	Rectangle(mDC, btnAtk.left, btnAtk.top, btnAtk.right, btnAtk.bottom);
	DrawText(mDC, _T("공격력 +"), -1, &btnAtk, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	Rectangle(mDC, btnSpeed.left, btnSpeed.top, btnSpeed.right, btnSpeed.bottom);
	DrawText(mDC, _T("이동속도 +"), -1, &btnSpeed, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

	Rectangle(mDC, btnRate.left, btnRate.top, btnRate.right, btnRate.bottom);
	DrawText(mDC, _T("공격속도 +"), -1, &btnRate, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    TCHAR atklevel[20];
    wsprintf(atklevel, _T("레벨 : %d"), unitAtk);
    SetTextColor(mDC, RGB(255, 0, 255));
    TextOut(mDC, btnAtk.right + 50, (btnAtk.bottom + btnAtk.top) / 2, atklevel, lstrlen(atklevel));

    TCHAR speedlevel[20];
    wsprintf(speedlevel, _T("레벨 : %d"), unitSpeed);
    TextOut(mDC, btnSpeed.right + 50, (btnSpeed.bottom + btnSpeed.top) / 2, speedlevel, lstrlen(speedlevel));

    TCHAR atk_speedlevel[20];
    wsprintf(atk_speedlevel, _T("레벨 : %d"), unitRate);
    TextOut(mDC, btnRate.right + 50, (btnRate.bottom + btnRate.top) / 2, atk_speedlevel, lstrlen(atk_speedlevel));

    RECT btnupgrade = { uis[3].x, 960 - uis[3].y - 150, uis[3].x + 150, 960 - uis[3].y - 100 };
    Rectangle(mDC, uis[3].x, 960 - uis[3].y - 150, uis[3].x + 150, 960 - uis[3].y - 100);
    SetTextColor(mDC, RGB(0, 0, 0));
    DrawText(mDC, _T("업그레이드 초기화"), -1, &btnupgrade, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
}

void DrawStageScreen(HDC mDC, HDC paintDC, int stageIdx,HWND hWnd) {
    if (hStageMap[stageIdx]) {
        HDC memDC = CreateCompatibleDC(mDC);
        SelectObject(memDC, hStageMap[stageIdx]);
        BitBlt(mDC, 0, 0, bmpStageMap[stageIdx].bmWidth, bmpStageMap[stageIdx].bmHeight, memDC, 0, 0, SRCCOPY);
        DeleteDC(memDC);
    }

    for (int i = 0; i < barbarianCount; i++) {
        int x = barbarians[i].x, y = barbarians[i].y, frame = barbarians[i].frameIndex;
        int tidx = barbarians[i].targetIndex;
        HBITMAP bmpToUse = hWalkBmp[frame];
        BITMAP* bmpInfo = &bmpWalk[frame];
        if (barbarians[i].hasTarget && tidx >= 0 && tidx < cannonCount[stageIdx]) {
            int cannonX = cannons[stageIdx][tidx].x;
            if (x < cannonX) {
                bmpToUse = hRightWalkBmp[frame];
                bmpInfo = &bmpRightWalk[frame];
            }
        }
        SelectObject(paintDC, bmpToUse);
        baba_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, x, y, tileHeight, tileHeight, paintDC, 0, 0,
            bmpInfo->bmWidth, bmpInfo->bmHeight, baba_back_color);
        DrawHealthBar(mDC, x, y, tileHeight, 6, barbarians[i].hp, barbarians[i].maxHp);
    }


    for (int i = 0; i < archerCount; i++) {
        int x = archers[i].x, y = archers[i].y, frame = archers[i].frameIndex;
        int tidx = archers[i].targetIndex;
        HBITMAP bmpToUse = hArcherBmp[frame];
        BITMAP* bmpInfo = &aWalk[frame];
        if (archers[i].hasTarget && tidx >= 0 && tidx < cannonCount[stageIdx]) {
            int cannonX = cannons[stageIdx][tidx].x;
            if (x < cannonX) {
                bmpToUse = hrightarcherbmp[frame];
                bmpInfo = &bmprightarcher[frame];
            }
        }
        SelectObject(paintDC, bmpToUse);
        COLORREF archer_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, x, y, tileHeight, tileHeight, paintDC, 0, 0,
            bmpInfo->bmWidth, bmpInfo->bmHeight, archer_back_color);
        DrawHealthBar(mDC, x, y, tileHeight, 6, archers[i].hp, archers[i].maxHp);
    }

    for (int i = 0; i < arrowEffectCount; i++) {
        if (!arrowEffects[i].active) continue;
        SelectObject(paintDC, harrowBmp);
        COLORREF arrow_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, arrowEffects[i].x, arrowEffects[i].y, tileHeight, tileHeight, paintDC, 0, 0,
            bmpArrow.bmWidth, bmpArrow.bmHeight, arrow_back_color);

    }

    for (int i = 0; i < giantsCount; i++) {
        int x = giants[i].x, y = giants[i].y, frame = giants[i].frameIndex;
        int tidx = giants[i].targetIndex;
        HBITMAP bmpToUse = hgiantsbmp[frame];
        BITMAP* bmpInfo = &bmpgiants[frame];
        if (giants[i].hasTarget && tidx >= 0 && tidx < cannonCount[stageIdx]) {
            int cannonX = cannons[stageIdx][tidx].x;
        }
        SelectObject(paintDC, bmpToUse);
        COLORREF giant_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, x, y, tileHeight*1.3f, tileHeight*1.3, paintDC, 0, 0,
            bmpInfo->bmWidth, bmpInfo->bmHeight, giant_back_color);
        DrawHealthBar(mDC, x, y, tileHeight, 6, giants[i].hp, giants[i].maxHp);
    }

    for (int i = 0; i < cannonCount[stageIdx]; i++) {
        if (!cannons[stageIdx][i].active) continue;
        SelectObject(paintDC, hCannonBmp);
        TransparentBlt(mDC, cannons[stageIdx][i].x, cannons[stageIdx][i].y,
            64, 64, paintDC, 0, 0,
            bmpCannon.bmWidth, bmpCannon.bmHeight, RGB(0, 0, 0));
        DrawHealthBar(mDC, cannons[stageIdx][i].x, cannons[stageIdx][i].y, 64, 6, cannons[stageIdx][i].hp, cannons[stageIdx][i].maxHp);
    }

    for (int i = 0; i <mortarCount[stageIdx]; i++) {
        if (!mortars[stageIdx][i].active) continue;
        SelectObject(paintDC, hmortarbmp);
        COLORREF mortar_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, mortars[stageIdx][i].x, mortars[stageIdx][i].y,
            64, 64, paintDC, 0, 0,
            bmpmortar.bmWidth, bmpmortar.bmHeight, mortar_back_color);
        DrawHealthBar(mDC, mortars[stageIdx][i].x, mortars[stageIdx][i].y, 64, 6, mortars[stageIdx][i].hp, mortars[stageIdx][i].maxHp);
    }
   
    for (int i = 0; i < infernoCount[stageIdx]; i++) {
        if (!infernos[stageIdx][i].active) continue;
        SelectObject(paintDC, hInfernoBmp);
        COLORREF inferno_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, infernos[stageIdx][i].x, infernos[stageIdx][i].y,
            64, 64, paintDC, 0, 0,
            bmpInferno.bmWidth, bmpInferno.bmHeight, inferno_back_color);
        DrawHealthBar(mDC, infernos[stageIdx][i].x, infernos[stageIdx][i].y, 64, 6, infernos[stageIdx][i].hp, infernos[stageIdx][i].maxHp);
    }
	for (int i = 0; i < cannonShotCount; i++) {
		shot* p = &cannonShots[i];
		if (!p->active) continue;

		// 탄 크기 결정
		int size = p->isMortarShot ? 24 : 10;

		// 위치 계산
		float drawX = p->x - size / 2.0f;  // 중앙 정렬
		float drawY = p->y - size / 2.0f;

		// 박격포는 포물선 궤적 보정
		if (p->isMortarShot) {
			float t = (float)(p->totalLife - p->life) / (float)p->totalLife;
			float height = -50.0f * t * (1.0f - t);
			drawY += height;
		}

		// 탄 그리기
		SelectObject(paintDC, hcannonBallBmp);
		COLORREF cannon_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, (int)drawX, (int)drawY, size, size, paintDC, 0, 0, 
        bmpCannonBall.bmWidth, bmpCannonBall.bmHeight, cannon_back_color);
    }


    
        
    
    //마을회관
    if (villageking.active)
    {
        SelectObject(paintDC, hvillagekingBmp);
        COLORREF  villageking_back_color = GetPixel(paintDC, 0, 1);
        TransparentBlt(mDC, villageking.x, villageking.y, 58, 58,
            paintDC, 0, 0,
            bmpvillageking.bmWidth, bmpvillageking.bmHeight, villageking_back_color);
        DrawHealthBar(mDC, villageking.x, villageking.y, 64, 6, villageking.hp, villageking.maxHp);
    }

    //캐릭터 ui창

    TextOut(mDC, uis[3].x + 70, uis[3].y + 60, timeStr, lstrlen(timeStr));

    SelectObject(paintDC, hui0bmp);
    COLORREF  ui0_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[0].x, uis[0].y, uis[0].length, uis[0].height, paintDC, 0, 0, bmpui0.bmWidth, bmpui0.bmHeight, ui0_back_color);

    SelectObject(paintDC, hui1bmp);
    COLORREF  ui1_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[1].x, uis[1].y, uis[1].length, uis[1].height, paintDC, 0, 0, bmpui1.bmWidth, bmpui1.bmHeight, ui1_back_color);
    TextOut(mDC, uis[1].x + 25, uis[1].y + uis[1].height - 5, "5", _tcslen("5"));
    MoveToEx(mDC, uis[1].x + 55, uis[1].y, NULL);
    LineTo(mDC, uis[1].x + 55, uis[1].y+uis[1].height);

    SelectObject(paintDC, hui2bmp);
    COLORREF  ui2_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[1].x+60, uis[1].y, uis[1].length, uis[1].height, paintDC, 0, 0, bmpui2.bmWidth, bmpui2.bmHeight, ui2_back_color);
    TextOut(mDC, uis[1].x + 85, uis[1].y + uis[1].height - 5, "5", _tcslen("5"));
    MoveToEx(mDC, uis[1].x + 115, uis[1].y, NULL);
    LineTo(mDC, uis[1].x + 115, uis[1].y + uis[1].height);

    SelectObject(paintDC, hui3bmp);
    COLORREF  ui3_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[1].x+120, uis[1].y, uis[1].length, uis[1].height, paintDC, 0, 0, bmpui3.bmWidth, bmpui3.bmHeight, ui3_back_color);
    TCHAR* str = _T("10");
    TextOut(mDC, uis[1].x + 145, uis[1].y + uis[1].height - 5, str,_tcslen(str));
    
    int mana_length;
    int earn = earn_star[0] + earn_star[1] + earn_star[2];
    if (earn < 3)
        mana_length = uis[2].length;
    else if (earn >= 3 && earn < 6)
        mana_length = uis[2].length+10;
    else if (earn >= 6)
        mana_length = uis[2].length+20;

    SelectObject(paintDC, hui4bmp);
    COLORREF  ui4_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[2].x , uis[2].y, mana_length, uis[2].height, paintDC, 0, 0, bmpui4.bmWidth, bmpui4.bmHeight, ui4_back_color);

    
    HPEN pen_mana = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH blue_mana = CreateSolidBrush(RGB(0, 0, 255));
    SelectObject(mDC, blue_mana);
    SelectObject(mDC, pen_mana);
    Rectangle(mDC, uis[2].x + 17, uis[2].y + 12, uis[2].x + 12 + mana, uis[2].y + 23);
    TCHAR buffer[30];
    wsprintf(buffer, L"%d / %d", mana,mana_length-10);
    TextOut(mDC, uis[2].x + 20, uis[2].y + 40, buffer, lstrlen(buffer));
    DeleteObject(blue_mana);

     //별
	if (towercount[stageIdx] >= naturaltowercount[stageIdx] * 0.7f) {
		// 첫 번째 별: 70% 이상일 때
		SelectObject(paintDC, hui5bmp);
		COLORREF ui5_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui5.bmWidth, bmpui5.bmHeight, ui5_back_color);
	}
	else {
		// 별 없을 때
		SelectObject(paintDC, hui6bmp);
		COLORREF ui6_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui6.bmWidth, bmpui6.bmHeight, ui6_back_color);
	}

	if (towercount[stageIdx] >= naturaltowercount[stageIdx] * 0.4f) {
		// 두 번째 별: 40% 이상일 때
		SelectObject(paintDC, hui5bmp);
		COLORREF ui5_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x + 60, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui5.bmWidth, bmpui5.bmHeight, ui5_back_color);
	}
	else {
		// 별 없을 때
		SelectObject(paintDC, hui6bmp);
		COLORREF ui6_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x + 60, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui6.bmWidth, bmpui6.bmHeight, ui6_back_color);
	}

	if (villageking.active) {
		// 마을 회관 활성화 시 별 표시
		SelectObject(paintDC, hui5bmp);
		COLORREF ui5_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x + 120, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui5.bmWidth, bmpui5.bmHeight, ui5_back_color);
	}
	else {
		// 마을 회관 비활성화 시 별 없거나 다른 표시
		SelectObject(paintDC, hui6bmp);
		COLORREF ui6_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x + 120, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui6.bmWidth, bmpui6.bmHeight, ui6_back_color);
		ui6_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x + 60, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui6.bmWidth, bmpui6.bmHeight, ui6_back_color);
		ui6_back_color = GetPixel(paintDC, 0, 1);
		TransparentBlt(mDC, uis[3].x, uis[3].y, uis[3].length, uis[3].height, paintDC, 0, 0, bmpui6.bmWidth, bmpui6.bmHeight, ui6_back_color);
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
            Unit* target = NULL;
            int idx = t->targetIndex;

            if (t->isTargetArcher && idx < archerCount)
                target = &archers[idx];
            else if (!t->isTargetArcher && idx < barbarianCount)
                target = &barbarians[idx];
            else if (!t->isTargetArcher && idx < giantsCount)
                target = &giants[idx];

            if (target && target->hp > 0) {
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
            t->targetIndex = -1;
        }

        // 새 타겟 탐색
        if (!t->isAttacking) {
            float minDist = 999999.0f;
            int bestIdx = -1;
            BOOL isArcher = FALSE, isGiant = FALSE;

            for (int j = 0; j < barbarianCount + archerCount + giantsCount; j++) {
                BOOL currIsArcher = (j >= barbarianCount && j < barbarianCount + archerCount);
                BOOL currIsGiant = (j >= barbarianCount + archerCount);

                if (infernoTargetUsed[j]) continue; // 다른 인페르노가 이미 타겟팅한 유닛이면 건너뜀

                Unit* target = currIsArcher ? &archers[j - barbarianCount] :
                                 (currIsGiant ? &giants[j - barbarianCount - archerCount] : &barbarians[j]);

                if (target->hp <= 0) continue;

                float dx = (target->x + tileWidth / 2) - (t->x + 32);
                float dy = (target->y + tileHeight / 2) - (t->y + 32);
                float dist = sqrtf(dx * dx + dy * dy);

                if (dist < 50 && dist < minDist) {
                    minDist = dist;
                    bestIdx = j;
                    isArcher = currIsArcher;
                    isGiant = currIsGiant;
                }
            }

            if (bestIdx != -1) {
                infernoTargetUsed[bestIdx] = TRUE;

                t->isAttacking = TRUE;
                t->isTargetArcher = isArcher;
                t->targetIndex = isArcher ? bestIdx - barbarianCount :
                                    (isGiant ? bestIdx - barbarianCount - archerCount : bestIdx);

                Unit* target = isArcher ? &archers[t->targetIndex] :
                                 (isGiant ? &giants[t->targetIndex] : &barbarians[t->targetIndex]);

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

                    infernoEffects[infernoEffectCount++] = (InfernoEffect){
                        sx, sy, dx / len, dy / len,
                        999, TRUE,
                        t->targetIndex,
                        isArcher,
                        isGiant,
                        i
                    };
                }
                t->attackCooldown = 1;
                valid = TRUE;
            }
        }

        // 데미지 처리
        if (t->isAttacking && valid) {
            Unit* target = NULL;
            if (t->isTargetArcher && t->targetIndex < archerCount)
                target = &archers[t->targetIndex];
            else if (!t->isTargetArcher && t->targetIndex < barbarianCount)
                target = &barbarians[t->targetIndex];
            else if (!t->isTargetArcher && t->targetIndex < giantsCount)
                target = &giants[t->targetIndex];

            if (target && target->hp > 0) {
                int damage = (int)(target->maxHp * t->damageRatio);
                if (damage < 1) damage = 1;
                target->hp -= damage;
                t->attackCooldown = 1;
            }
        }
    }
}

// 인페르노 공격 그리기
void DrawInfernoEffects(HDC mDC, int stageIdx) {
	for (int i = 0; i < infernoEffectCount; i++) {
		if (!infernoEffects[i].active) continue;

		int towerId = infernoEffects[i].sourceTowerId;
		if (!infernos[stageIdx][towerId].isAttacking) {
			infernoEffects[i].active = FALSE;
			continue;
		}

		float sx = infernoEffects[i].x;
		float sy = infernoEffects[i].y;

		int tidx = infernoEffects[i].targetIndex;
		BOOL isArcher = infernoEffects[i].isArcher;

		float tx = 0, ty = 0;
		if (isArcher && tidx < archerCount) {
			tx = archers[tidx].x + tileWidth / 2;
			ty = archers[tidx].y + tileHeight / 2;
		}
		else if (!isArcher && !infernoEffects[i].isGiant && tidx < barbarianCount) {
			tx = barbarians[tidx].x + tileWidth / 2;
			ty = barbarians[tidx].y + tileHeight / 2;
		}
		else if (!isArcher && infernoEffects[i].isGiant && tidx < giantsCount) {
			tx = giants[tidx].x + tileWidth / 2;
			ty = giants[tidx].y + tileHeight / 2;
		}
		else {
			infernoEffects[i].active = FALSE;
			continue;
		}

		HPEN hPen = CreatePen(PS_SOLID, 4, RGB(255, 253, 126));
		HPEN oldPen = SelectObject(mDC, hPen);

		MoveToEx(mDC, (int)sx, (int)sy, NULL);
		LineTo(mDC, (int)tx, (int)ty);

		SelectObject(mDC, oldPen);
		DeleteObject(hPen);

		infernoEffects[i].life--;
		if (infernoEffects[i].life <= 0) {
			infernoEffects[i] = infernoEffects[--infernoEffectCount];
			i--;
		}
	}
}



void DRAWENDScreen(HWND hWnd,HDC mDC,HDC paintDC)
{
    RECT bkrect;
    GetClientRect(hWnd, &bkrect);
    if (victory)
    {
        SelectObject(paintDC, endvbmp);
        StretchBlt(mDC, 0, 0, bkrect.right, bkrect.bottom, paintDC, 0, 0, bmpendv.bmWidth, bmpendv.bmHeight, SRCCOPY);
        HFONT hFont = CreateFont( 60, 0,0, 0,FW_BOLD,FALSE, FALSE, FALSE,DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,  DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS,TEXT("맑은 고딕") );

        HFONT hOldFont = (HFONT)SelectObject(mDC, hFont);

        SetBkMode(mDC, TRANSPARENT);       
        SetTextColor(mDC, RGB(0, 0, 0));   
        RECT rect = { 300, 650, 800, 780 };
        DrawText(mDC,TEXT("게임승리!\n아무 곳이나 클릭하세요"),-1, &rect,DT_LEFT | DT_TOP | DT_WORDBREAK );

        SelectObject(mDC, hOldFont); 
        DeleteObject(hFont);
    }

    else if(!victory && gameend)
    {
        SelectObject(paintDC, enddbmp);
        StretchBlt(mDC, 0, 0, bkrect.right, bkrect.bottom, paintDC, 0, 0, bmpendd.bmWidth, bmpendd.bmHeight, SRCCOPY);
        HFONT hFont = CreateFont(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("맑은 고딕"));

        HFONT hOldFont = (HFONT)SelectObject(mDC, hFont);

        SetBkMode(mDC, TRANSPARENT); 
        SetTextColor(mDC, RGB(0, 0, 0));   
        RECT rect = { 300, 650, 800, 780 };
        DrawText(mDC, TEXT("게임패배..\n아무 곳이나 클릭하세요"), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);
        
        SelectObject(mDC, hOldFont); 
        DeleteObject(hFont);
    }
}

void DrawGrid(HDC hDC) {
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

    int startX = 670;
    int startY = 110;

    for (int row = 0; row < boardSize; row++) {
        for (int col = 0; col < boardSize; col++) {
            int centerX = startX + (col - row) * (tileWidth / 2);
            int centerY = startY + (col + row) * (tileHeight / 2);

            POINT p[5];
            p[0].x = centerX;
            p[0].y = centerY - tileHeight / 2;
            p[1].x = centerX + tileWidth / 2;
            p[1].y = centerY;
            p[2].x = centerX;
            p[2].y = centerY + tileHeight / 2;
            p[3].x = centerX - tileWidth / 2;
            p[3].y = centerY;
            p[4] = p[0];

            MoveToEx(hDC, p[0].x, p[0].y, NULL);
            for (int i = 1; i < 5; i++) {
                LineTo(hDC, p[i].x, p[i].y);
            }
        }
    }

    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

void Drawtowerrange(HDC hDC,int stageIdx) {
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(200, 130, 35));
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
    HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hDC, hbrush);


    for (int i = 0; i < cannonCount[stageIdx]; i++) {
        tower* c = &cannons[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x+tileWidth -100, c->y + tileWidth - 100, c->x + tileWidth + 100, c->y + tileWidth + 100);
    }

    DeleteObject(hPen);
    hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    SelectObject(hDC, hPen);

    for (int i = 0; i < mortarCount[stageIdx]; i++) {
        tower* c = &mortars[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x + tileWidth -200, c->y + tileWidth - 200, c->x + tileWidth + 200, c->y + tileWidth + 200);
    }

    DeleteObject(hPen);
    hPen = CreatePen(PS_SOLID, 2, RGB(242, 78, 4));
    SelectObject(hDC, hPen);

    for (int i = 0; i < infernoCount[stageIdx]; i++) {
        tower* c = &infernos[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x + tileWidth - 50, c->y + tileWidth - 50, c->x + tileWidth + 50, c->y + tileWidth + 50);
    }

    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

void Drawblock(HDC mDC, int stageIdx)
{
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    SelectObject(mDC, hPen);
    HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(mDC, hbrush);

    int startX = 670;
    int startY = 110;

    Rectangle(mDC, villageking.x, villageking.y, villageking.x + 64, villageking.y + 64);

    for (int i = 0; i < cannonCount[gameState - STATE_STAGE1]; i++)
    {
    Rectangle(mDC, cannons[stageIdx][i].x, cannons[stageIdx][i].y, cannons[stageIdx][i].x + 64, cannons[stageIdx][i].y + 64);
    MoveToEx(mDC, cannons[stageIdx][i].x, cannons[stageIdx][i].y, NULL);
    LineTo(mDC, cannons[stageIdx][i].x + 64, cannons[stageIdx][i].y + 64);
    MoveToEx(mDC, cannons[stageIdx][i].x, cannons[stageIdx][i].y + 64, NULL);
    LineTo(mDC, cannons[stageIdx][i].x + 64, cannons[stageIdx][i].y);
    }

    for (int i = 0; i < mortarCount[stageIdx]; i++)
    {
        Rectangle(mDC, mortars[stageIdx][i].x, mortars[stageIdx][i].y, mortars[stageIdx][i].x + 64, mortars[stageIdx][i].y + 64);
        MoveToEx(mDC, mortars[stageIdx][i].x, mortars[stageIdx][i].y, NULL);
        LineTo(mDC, mortars[stageIdx][i].x + 64, mortars[stageIdx][i].y + 64);
        MoveToEx(mDC, mortars[stageIdx][i].x, mortars[stageIdx][i].y + 64, NULL);
        LineTo(mDC, mortars[stageIdx][i].x + 64, mortars[stageIdx][i].y);
    }

    for (int i = 0; i < infernoCount[stageIdx]; i++)
    {
        Rectangle(mDC, infernos[stageIdx][i].x, infernos[stageIdx][i].y, infernos[stageIdx][i].x + 64, infernos[stageIdx][i].y + 64);
        MoveToEx(mDC, infernos[stageIdx][i].x, infernos[stageIdx][i].y, NULL);
        LineTo(mDC, infernos[stageIdx][i].x + 64, infernos[stageIdx][i].y + 64);
        MoveToEx(mDC, infernos[stageIdx][i].x, infernos[stageIdx][i].y + 64, NULL);
        LineTo(mDC, infernos[stageIdx][i].x + 64, infernos[stageIdx][i].y);
    }
    DeleteObject(hPen);

}
void UpdateUnlockStatus() {
    int star = earn_star[0] + earn_star[1] + earn_star[2];
	stage2Unlocked = unlockOverride || (star >= 3);
	stage3Unlocked = unlockOverride || (star >= 6);
	archerUnlocked = unlockOverride || (star >= 3);
	giantUnlocked = unlockOverride || (star >= 6);
}
void stage_end(HWND hWnd,int stageIdx)
{
    int starPlus = 0;

    if (!villageking.active && !gameend)
    {
        gameState = STATE_END;
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
    else if (_tcscmp(timeStr, TEXT("00:00")) == 0)
    {
        gameState = STATE_END;
        gameend = TRUE;
        victory = FALSE;

        if (starEarned[stageIdx]) return;

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
}

void UpdateUnit(Unit* u, tower* cannons, tower* mortars, InfernoTower* infernos, int infernoCount, int mortarCount, int cannonCount, BOOL isRanged, int stageIdx) {
    if (u->hp <= 0) return;

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
        tower* c = &infernos[u->targetIndex];
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



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    HWND hWnd;
    MSG Message;
    WNDCLASSEX WndClass = { 0 };
    g_hInst = hInstance;

    WndClass.cbSize = sizeof(WNDCLASSEX);
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = WndProc;
    WndClass.hInstance = hInstance;
    WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    WndClass.lpszClassName = lpszClass;
    RegisterClassEx(&WndClass);

    hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 1280, 940, NULL, NULL, hInstance, NULL);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    SetTimer(hWnd, IDT_LOADING_TIMER, 100, NULL);
    SetTimer(hWnd, IDT_ANIM_TIMER, 200, NULL);

    while (GetMessage(&Message, 0, 0, 0)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
    static HBITMAP hmDC;
    static HDC mDC, paintDC;

    switch (iMessage) {
    case WM_CREATE:
        PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
        if (loadingProgress >= 100) {
            isLoading = FALSE;
            gameState = STATE_MENU;
            KillTimer(hWnd, IDT_LOADING_TIMER);
            PlaySound(NULL, 0, 0);
        }
        InitGame(hWnd);
        //isLoading = FALSE; // 주석처리로 로딩 끄기 가능
        //gameState = STATE_MENU;
        shopButton = (RECT){ 50, 800, 200, 860 };  // 좌측 하단 대략 위치
        break;

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam), y = HIWORD(lParam);

        if (gameState == STATE_SHOP) {
            if (PtInRect(&shopExitButton, (POINT) { x, y })) {
                gameState = STATE_MENU;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            POINT pt = { x, y };
                
            RECT init_upgrade = { uis[3].x, 960 - uis[3].y - 150, uis[3].x + 150, 960 - uis[3].y-100 };

			if (PtInRect(&btnAtk, pt)) {
				int cost = unitAtk + 1;
				if (starCount >= cost && unitAtk < 3) {
					starCount -= cost;
					unitAtk++;
				}
			}
			else if (PtInRect(&btnSpeed, pt)) {
				int cost = unitSpeed + 1;
				if (starCount >= cost && unitSpeed < 3) {
					starCount -= cost;
					unitSpeed++;
				}
			}
			else if (PtInRect(&btnRate, pt)) {
				int cost = unitRate + 1;
				if (starCount >= cost && unitRate < 3) {
					starCount -= cost;
					unitRate++;
				}
			}
            else if (PtInRect(&init_upgrade, pt)) {
                unitAtk = 0;
                unitSpeed = 0;
                unitRate = 0;
                starCount = earn_star[0] + earn_star[1] + earn_star[2];
            }

            InvalidateRect(hWnd, NULL, FALSE); 
            return 0;
        }

        if (gameState == STATE_MENU) {
            for (int i = 0; i < DOOR_COUNT; i++) {
                if (PtInRect(&doorRects[i], (POINT) { x, y })) {
					if ((i == 1 && !archerUnlocked) || (i == 2 && !giantUnlocked)) {
						MessageBox(hWnd, _T("해당 스테이지는 아직 해금되지 않았습니다."), _T("잠김"), MB_OK | MB_ICONWARNING);
						return 0;
					}
                    barbarianPos.x = doorRects[i].left - 80;
                    barbarianPos.y = doorRects[i].top + 40;
                    barbarianEntering[i] = TRUE;
                    enteringIndex = i;
                    stagesoundplay = FALSE;
                    InvalidateRect(hWnd, NULL, FALSE);
                    return 0;
                }
            }
            for (int i = 0; i < 3; i++) {
                if (PtInRect(&stageButtons[i], (POINT) { x, y })) {
                    gameState = STATE_STAGE1 + i;
                    barbarianCount = 0;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            if (PtInRect(&shopButton, (POINT) { x, y })) {
                gameState = STATE_SHOP;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
        }

        if (gameState == STATE_END)
        {
            gameState = STATE_MENU;
            InvalidateRect(hWnd, NULL, FALSE);
            InitGame(hWnd);
            break;
        }

        else if (gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3 && (barbarianCount < MAX_UNITS || archerCount < MAX_UNITS)) {
            int stage = 0;
            if (gameState == 2)
                stage = 1;
            else if (gameState == 3)
                stage = 2;
            else if (gameState == 4)
                stage = 3;

            POINT pt = { x, y };
            
            int bx = x, by = y;
            int minDist = 999999;
            int tx = -1, ty = -1;
            int stageIdx = gameState - STATE_STAGE1;
            int targetIdx = -1;
            BOOL tower_on_unit = FALSE;

            RECT tower = { villageking.x,villageking.y,villageking.x + 64,villageking.y + 64 };
            tower_on_unit = PtInRect(&tower, pt);
            if (!tower_on_unit)
            {
                switch (stageIdx)
                {
                case 0:
                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        RECT tower = { cannons[stageIdx][i].x,cannons[stageIdx][i].y,cannons[stageIdx][i].x + 64,cannons[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { mortars[stageIdx][i].x,mortars[stageIdx][i].y,mortars[stageIdx][i].x + 64,mortars[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { infernos[stageIdx][i].x,infernos[stageIdx][i].y,infernos[stageIdx][i].x + 64,infernos[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }
                    break;

                case 1:
                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        RECT tower = { cannons[stageIdx][i].x,cannons[stageIdx][i].y,cannons[stageIdx][i].x + 64,cannons[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { mortars[stageIdx][i].x,mortars[stageIdx][i].y,mortars[stageIdx][i].x + 64,mortars[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { infernos[stageIdx][i].x,infernos[stageIdx][i].y,infernos[stageIdx][i].x + 64,infernos[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }
                    break;

                case 2:
                    for (int i = 0; i < cannonCount[stageIdx]; i++)
                    {
                        RECT tower = { cannons[stageIdx][i].x,cannons[stageIdx][i].y,cannons[stageIdx][i].x + 64,cannons[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < mortarCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { mortars[stageIdx][i].x,mortars[stageIdx][i].y,mortars[stageIdx][i].x + 64,mortars[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }

                    for (int i = 0; i < infernoCount[stageIdx]; i++)
                    {
                        if (tower_on_unit)
                            break;
                        RECT tower = { infernos[stageIdx][i].x,infernos[stageIdx][i].y,infernos[stageIdx][i].x + 64,infernos[stageIdx][i].y + 64 };
                        tower_on_unit = PtInRect(&tower, pt);
                        if (tower_on_unit)
                            break;
                    }
                    break;

                }
            }

            for (int i = 0; i < cannonCount[stageIdx]; i++) {
                if (!cannons[stageIdx][i].active) continue;
                int dx = cannons[stageIdx][i].x - bx;
                int dy = cannons[stageIdx][i].y - by;
                int dist = dx * dx + dy * dy;
                if (dist < minDist) {
                    minDist = dist;
                    tx = cannons[stageIdx][i].x;
                    ty = cannons[stageIdx][i].y;
                    targetIdx = i;
                }
            }
            if (!tower_on_unit)
            {
                if (mana - 5 >= 0)
                {
                    // Q 키 → 바바리안
                    if (GetAsyncKeyState('Q') & 0x8000 && barbarianCount < MAX_UNITS) {
                        barbarians[barbarianCount++] = (Unit){
                            x, y, 0, 7 + unitSpeed, {0, 0}, 100, 100, 10 + unitAtk, 25,
                            tx, ty, (tx != -1), targetIdx, 0,
                            0
                        };
                        mana -= 5;
                    }

                    // W 키 → 아처
                    else if (GetAsyncKeyState('W') & 0x8000 && archerCount < MAX_UNITS) {
						if (!archerUnlocked) return 0;
                        archers[archerCount++] = (Unit){
                            x, y, 0, 7 + unitSpeed, {0, 0}, 80, 80, 15 + unitAtk, 75,
                            tx, ty, (tx != -1), targetIdx, 0,
                            0
                        };
                        mana -= 5;
                    }
                }

                if (GetAsyncKeyState('E') & 0x8000 && giantsCount < MAX_UNITS && mana - 10 >= 0) {
					if (!giantUnlocked) return 0;
                    giants[giantsCount++] = (Unit){
                        x, y, 0, 4 + unitSpeed, {0, 0}, 500, 500, 25 + unitAtk, 25,
                        tx, ty, (tx != -1), targetIdx, 0,
                        0
                    };
                    mana -= 10;
                }

                if (!stagestart)
                {
                    SetTimer(hWnd, IDT_STAGE_TIMER, 1000, NULL);
                    stagestart = TRUE;
                }

                InvalidateRect(hWnd, NULL, FALSE);
            }

        }
		break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_TOOLS_RESET:
            reset_game(hWnd);
            break;

        case ID_SPEED_FAST:
            KillTimer(hWnd, IDT_ANIM_TIMER);
            SetTimer(hWnd, IDT_ANIM_TIMER, 100, NULL);
            KillTimer(hWnd, IDT_STAGE_TIMER);
            SetTimer(hWnd, IDT_STAGE_TIMER, 500, NULL);
            break;

        case ID_SPEED_SLOW:
            KillTimer(hWnd, IDT_ANIM_TIMER);
            SetTimer(hWnd, IDT_ANIM_TIMER, 400, NULL);
            KillTimer(hWnd, IDT_STAGE_TIMER);
            SetTimer(hWnd, IDT_STAGE_TIMER, 2000, NULL);
            break;

        case ID_SPEED_NORMAL:
            KillTimer(hWnd, IDT_ANIM_TIMER);
            SetTimer(hWnd, IDT_ANIM_TIMER, 200, NULL);
            KillTimer(hWnd, IDT_STAGE_TIMER);
            SetTimer(hWnd, IDT_STAGE_TIMER, 1000, NULL);
            break;

        }
        break;

    case WM_KEYDOWN:
    {
        int stage_idx = gameState - 1;
        
        if (wParam == VK_ESCAPE && gameState != STATE_LOADING) {
            gameState = STATE_MENU;
            InitGame(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if ((stage_idx == 1 || stage_idx == 2 || stage_idx == 3) && (wParam == 'X' || wParam == 'x')) {
            Stagetime = 5;
        }

        else if (wParam == 'p' || wParam == 'P')
        {
            if (!showGrid)
                showGrid = TRUE;
            else
                showGrid = FALSE;
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if (wParam == 'm' || wParam == 'M')
        {
            if (!showrange)
                showrange = TRUE;
            else
                showrange = FALSE;
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if (wParam == 'b' || wParam == 'B')
        {
            if (!showblock)
                showblock = TRUE;
            else
                showblock = FALSE;
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if (wParam == 'r' || wParam == 'R')
        {
            InitGame(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if (wParam == 'u' || wParam == 'U') {
			unlockOverride = !unlockOverride;
			UpdateUnlockStatus();
			InvalidateRect(hWnd, NULL, TRUE); 
			break;
		}
        break;
    }

    case WM_TIMER:
		if (gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3)
			stage_end(hWnd, gameState - STATE_STAGE1);

        if (wParam == IDT_STAGE_TIMER && gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3)
        {
            if (Stagetime > 0) {
                Stagetime--;

                int minutes = Stagetime / 60;
                int seconds = Stagetime % 60;

                wsprintf(timeStr, TEXT("%02d:%02d"), minutes, seconds);
            }
            else {
                KillTimer(hWnd, 1);
                wsprintf(timeStr, TEXT("00:00"));
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }

        if (wParam == IDT_LOADING_TIMER && isLoading) {
            loadingProgress += 5;
            if (loadingProgress >= 100) {
                isLoading = FALSE;
                gameState = STATE_MENU;
                KillTimer(hWnd, IDT_LOADING_TIMER);
            }
            InvalidateRect(hWnd, NULL, FALSE);
        }

        else if (wParam == IDT_ANIM_TIMER && gameState == STATE_MENU)
        {
            if (enteringIndex != -1) {
                if (barbarianEntering[enteringIndex]) {
                    int dx = (doorRects[enteringIndex].left + 32) - (barbarianPos.x + tileWidth / 2);
                    int dy = (doorRects[enteringIndex].top + 32) - (barbarianPos.y + tileHeight / 2);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 5.0f) {
                        barbarianPos.x += (int)(dx / dist * 10);
                        barbarianPos.y += (int)(dy / dist * 10);
                    }
                    else {
                        doorOpenings[enteringIndex] = TRUE;
                    }
                }

                if (doorOpenings[enteringIndex]) {
                    if (doorFrameIndices[enteringIndex] < 4) {
                        doorFrameIndices[enteringIndex]++;
                    }
                    else {
                        gameState = targetStages[enteringIndex];
                        barbarianCount = archerCount = 0;
                        currentStageIdx = targetStages[enteringIndex] - STATE_STAGE1;
                        doorOpenings[enteringIndex] = FALSE;
                        barbarianEntering[enteringIndex] = FALSE;
                        doorFrameIndices[enteringIndex] = 0;
                        enteringIndex = -1;
                    }
                }

                barbarianMenuFrame = (barbarianMenuFrame + 1) % 3;
            }
        }

        else if (wParam == IDT_ANIM_TIMER && gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3) {
            int stageIdx = gameState - STATE_STAGE1;

            for (int i = 0; i < cannonCount[stageIdx]; i++) {
                if (cannons[stageIdx][i].attackCooldown > 0)
                    cannons[stageIdx][i].attackCooldown--;
            }
            // [1] 바바리안 제거 (hp <= 0)
            for (int i = 0; i < barbarianCount; ) {
                if (barbarians[i].hp <= 0) {
                    for (int j = i; j < barbarianCount - 1; j++) {
                        barbarians[j] = barbarians[j + 1];
                    }
                    barbarianCount--;
                }
                else {
                    i++;
                }
            }

            // [2] 아처 제거 (hp <= 0)
            for (int i = 0; i < archerCount; ) {
                if (archers[i].hp <= 0) {
                    for (int j = i; j < archerCount - 1; j++) {
                        archers[j] = archers[j + 1];
                    }
                    archerCount--;
                }
                else {
                    i++;
                }
            }

            //자이언트 제거 hp 0 아래일때
            for (int i = 0; i < giantsCount; ) {
                if (giants[i].hp <= 0) {
                    for (int j = i; j < giantsCount - 1; j++) {
                        giants[j] = giants[j + 1];
                    }
                    giantsCount--;
                }
                else {
                    i++;
                }
            }

            // 유닛 프레임 애니메이션
            for (int i = 0; i < barbarianCount; i++)
                barbarians[i].frameIndex = (barbarians[i].frameIndex + 1) % 3;

            for (int i = 0; i < archerCount; i++)
                archers[i].frameIndex = (archers[i].frameIndex + 1) % 3;

            giant_frame++;
            if (giant_frame % 2 == 0)
            {
                for (int i = 0; i < giantsCount; i++)
                    giants[i].frameIndex = (giants[i].frameIndex + 1) % 3;
            }


            //바바 이동 공격
            for (int i = 0; i < barbarianCount; i++)
                UpdateUnit(&barbarians[i], cannons[stageIdx], mortars[stageIdx], infernos[stageIdx], infernoCount[stageIdx], mortarCount[stageIdx], cannonCount[stageIdx], FALSE, stageIdx);

            //아처 이동 공격
            for (int i = 0; i < archerCount; i++)
                UpdateUnit(&archers[i], cannons[stageIdx], mortars[stageIdx], infernos[stageIdx], infernoCount[stageIdx], mortarCount[stageIdx], cannonCount[stageIdx], TRUE, stageIdx);

            //자이언트 //
            for (int i = 0; i < giantsCount; i++)
                UpdateUnit(&giants[i], cannons[stageIdx], mortars[stageIdx], infernos[stageIdx], infernoCount[stageIdx], mortarCount[stageIdx], cannonCount[stageIdx], FALSE, stageIdx);

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
                        tower* c = &infernos[stageIdx][arrow->targetIndex];
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

            // [8] 대포 -> 범위 내 유닛에게 대포알 발사
            for (int i = 0; i < cannonCount[stageIdx]; i++) {
                tower* c = &cannons[stageIdx][i];
                if (!c->active) continue;

                // 가장 가까운 유닛 탐색
                float minDist = 999999.0f;
                int targetIndex = -1;
                int u_type = -1;

                // 바바리안 검사
                for (int j = 0; j < barbarianCount; j++) {
                    float dx = (barbarians[j].x + tileWidth / 2) - (c->x + tileWidth);
                    float dy = (barbarians[j].y + tileHeight / 2) - (c->y + tileWidth);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < 100 && dist < minDist) {
                        minDist = dist;
                        targetIndex = j;
                        u_type = 0;
                    }
                }

                // 아처 검사
                for (int j = 0; j < archerCount; j++) {
                    float dx = (archers[j].x + tileWidth / 2) - (c->x + tileWidth);
                    float dy = (archers[j].y + tileHeight / 2) - (c->y + tileWidth);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < 100 && dist < minDist) {
                        minDist = dist;
                        targetIndex = j;
                        u_type = 1;
                    }
                }

                //자이언트 검사
                for (int j = 0; j < giantsCount; j++) {
                    float dx = (giants[j].x + tileWidth / 2) - (c->x + tileWidth);
                    float dy = (giants[j].y + tileHeight / 2) - (c->y + tileWidth);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist < 100 && dist < minDist) {
                        minDist = dist;
                        targetIndex = j;
                        u_type = 2;
                    }
                }

                // 유닛 발견 시 대포알 발사
                if (targetIndex != -1 && c->attackCooldown == 0) {
                    shot shot;
                    shot.x = c->x + 32;
                    shot.y = c->y + 32;
                    shot.targetIndex = targetIndex;
                    shot.unittype = u_type;
                    shot.active = TRUE;
                    shot.life = 60;
                    shot.isMortarShot = FALSE;

                    float tx;
                    float ty;
                    switch (shot.unittype)
                    {
                    case 0:
                        tx = barbarians[shot.targetIndex].x + tileWidth / 2;
                        ty = barbarians[shot.targetIndex].y + tileHeight / 2;
                        break;
                    case 1:
                        tx = archers[shot.targetIndex].x + tileWidth / 2;
                        ty = archers[shot.targetIndex].y + tileHeight / 2;

                        break;
                    case 2:
                        tx = giants[shot.targetIndex].x + tileWidth / 2;
                        ty = giants[shot.targetIndex].y + tileHeight / 2;

                        break;
                    }

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

				// 타겟 유닛이 죽었거나 잘못된 인덱스면 탄 제거
				if (p->unittype == 0 && (p->targetIndex >= barbarianCount || barbarians[p->targetIndex].hp <= 0)) {
					cannonShots[i] = cannonShots[--cannonShotCount];
					continue;
				}
				if (p->unittype == 1 && (p->targetIndex >= archerCount || archers[p->targetIndex].hp <= 0)) {
					cannonShots[i] = cannonShots[--cannonShotCount];
					continue;
				}
				if (p->unittype == 2 && (p->targetIndex >= giantsCount || giants[p->targetIndex].hp <= 0)) {
					cannonShots[i] = cannonShots[--cannonShotCount];
					continue;
				}

				//  현재 위치로 충돌 체크
				float tx = 0, ty = 0;
				if (p->unittype == 0) {
					tx = barbarians[p->targetIndex].x + tileWidth / 2;
					ty = barbarians[p->targetIndex].y + tileHeight / 2;
				}
				else if (p->unittype == 1) {
					tx = archers[p->targetIndex].x + tileWidth / 2;
					ty = archers[p->targetIndex].y + tileHeight / 2;
				}
				else if (p->unittype == 2) {
					tx = giants[p->targetIndex].x + tileWidth / 2;
					ty = giants[p->targetIndex].y + tileHeight / 2;
				}

				float dx = p->x - tx;
				float dy = p->y - ty;
				float dist = sqrtf(dx * dx + dy * dy);
				int cannondamage = 50;

				if (dist < 10.0f || p->life <= 0) {
					if (p->unittype == 0)
						barbarians[p->targetIndex].hp -= cannondamage;
					else if (p->unittype == 1)
						archers[p->targetIndex].hp -= cannondamage;
					else if (p->unittype == 2)
						giants[p->targetIndex].hp -= cannondamage;

					cannonShots[i] = cannonShots[--cannonShotCount];
					continue;
				}

				i++;  // 다음 탄으로
			}

            //박격포
            for (int i = 0; i < mortarCount[stageIdx]; i++) {
                tower* m = &mortars[stageIdx][i];
                if (!m->active) continue;
                if (m->attackCooldown > 0) {
                    m->attackCooldown--;
                    continue;
                }

                float maxDist = -1.0f;
                int targetIndex = -1;
                int u_type = -1;
                float tx = 0, ty = 0;

                // 바바리안
                for (int j = 0; j < barbarianCount; j++) {
                    if (barbarians[j].hp <= 0) continue;

                    float dx = (barbarians[j].x + tileWidth / 2) - (m->x + tileWidth / 2);
                    float dy = (barbarians[j].y + tileHeight / 2) - (m->y + tileHeight / 2);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 60 && dist > maxDist && dist <= m->range) {
                        maxDist = dist;
                        targetIndex = j;
                        u_type = 0;
                        tx = barbarians[j].x + tileWidth / 2;
                        ty = barbarians[j].y + tileHeight / 2;
                    }
                }

                // 아처
                for (int j = 0; j < archerCount; j++) {
                    if (archers[j].hp <= 0) continue;

                    float dx = (archers[j].x + tileWidth / 2) - (m->x + tileWidth / 2);
                    float dy = (archers[j].y + tileHeight / 2) - (m->y + tileHeight / 2);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 60 && dist > maxDist && dist <= m->range) {
                        maxDist = dist;
                        targetIndex = j;
                        u_type = 1;
                        tx = archers[j].x + tileWidth / 2;
                        ty = archers[j].y + tileHeight / 2;
                    }
                }

                // 자이언트
                for (int j = 0; j < giantsCount; j++) {
                    if (giants[j].hp <= 0) continue;

                    float dx = (giants[j].x + tileWidth / 2) - (m->x + tileWidth / 2);
                    float dy = (giants[j].y + tileHeight / 2) - (m->y + tileHeight / 2);
                    float dist = sqrtf(dx * dx + dy * dy);
                    if (dist > 60 && dist > maxDist && dist <= m->range) {
                        maxDist = dist;
                        targetIndex = j;
                        u_type = 2;
                        tx = giants[j].x + tileWidth / 2;
                        ty = giants[j].y + tileHeight / 2;
                    }
                }

                if (targetIndex != -1 && cannonShotCount < MAX_EFFECTS) {
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
                    s.targetIndex = targetIndex;
                    s.unittype = u_type;
                    s.isMortarShot = TRUE;

                    cannonShots[cannonShotCount++] = s;
                    m->attackCooldown = 30;
                }
            }
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

					// 바바리안 전체 검사
					for (int j = 0; j < barbarianCount; j++) {
						float bx = barbarians[j].x + tileWidth / 2;
						float by = barbarians[j].y + tileHeight / 2;
						float d = sqrtf((bx - tx) * (bx - tx) + (by - ty) * (by - ty));
						if (d <= 30) {
							barbarians[j].hp -= mortardamage;
						}
					}

					// 아처 전체 검사
					for (int j = 0; j < archerCount; j++) {
						float ax = archers[j].x + tileWidth / 2;
						float ay = archers[j].y + tileHeight / 2;
						float d = sqrtf((ax - tx) * (ax - tx) + (ay - ty) * (ay - ty));
						if (d <= 30) {
							archers[j].hp -= mortardamage;
						}
					}

					// 자이언트 전체 검사
					for (int j = 0; j < giantsCount; j++) {
						float gx = giants[j].x + tileWidth / 2;
						float gy = giants[j].y + tileHeight / 2;
						float d = sqrtf((gx - tx) * (gx - tx) + (gy - ty) * (gy - ty));
						if (d <= 30) {
							giants[j].hp -= mortardamage;
						}
					}

					cannonShots[i] = cannonShots[--cannonShotCount];
					continue; 
				}

				i++;
			}

            InfernoAttack(stageIdx);
        }
        InvalidateRect(hWnd, NULL, FALSE);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        mDC = CreateCompatibleDC(hDC);
        paintDC = CreateCompatibleDC(hDC);
        hmDC = CreateCompatibleBitmap(hDC, 1280, 940);
        SelectObject(mDC, hmDC);

        if (isLoading)
            DrawLoadingScreen(hDC, mDC, 1280, 940);
        else if (gameState == STATE_MENU)
        {
            DrawMenuScreen(mDC, 1280, 940);
            if (!menusoundplay)
            {
                PlaySound(0, 0, NULL);
                PlaySound(MAKEINTRESOURCE(IDR_WAVE2), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
                menusoundplay = TRUE;
            }
        }
        else if (gameState == STATE_SHOP)
            DrawShopScreen(mDC);
        else if (gameState == STATE_END)
            DRAWENDScreen(hWnd, mDC, paintDC);
        else {
            if (!stagesoundplay)
            {
                PlaySound(0, 0, NULL);
                PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
                stagesoundplay = TRUE;
            }
            DrawStageScreen(mDC, paintDC, gameState - STATE_STAGE1, hWnd);
            if (showGrid)
                DrawGrid(mDC);
            if (showrange)
                Drawtowerrange(mDC,gameState - STATE_STAGE1);
            if (showblock)
                Drawblock(mDC, gameState - STATE_STAGE1);
            DrawInfernoEffects(mDC, gameState - STATE_STAGE1);
        }



        BitBlt(hDC, 0, 0, 1280, 940, mDC, 0, 0, SRCCOPY);
        DeleteDC(mDC);
        DeleteDC(paintDC);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        for (int i = 0; i < 3; i++) {
            if (hWalkBmp[i]) DeleteObject(hWalkBmp[i]);
            if (hStageMap[i]) DeleteObject(hStageMap[i]);
        }
        if (hLogoBmp) DeleteObject(hLogoBmp);
        if (hCannonBmp) DeleteObject(hCannonBmp);
        PlaySound(0, 0, NULL);
        KillTimer(hWnd, IDT_ANIM_TIMER);
        KillTimer(hWnd, IDT_LOADING_TIMER);
        KillTimer(hWnd, IDT_STAGE_TIMER);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
 }
