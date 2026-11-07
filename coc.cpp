#include <windows.h>
#include <tchar.h>
#include <stdbool.h>
#include <math.h>
#include "resource.h"
#include "BattleLogic.h"
#include "RendererGlobals.h"
#include "Renderer.h"
#include "GameFramework.h"
#include <mmsystem.h>

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"Window Programming Lab";



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
tower villageking = { 0 };

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


int doorFrameIndices[DOOR_COUNT] = { 0, 0, 0 };
BOOL doorOpenings[DOOR_COUNT] = { FALSE, FALSE, FALSE };
BOOL barbarianEntering[DOOR_COUNT] = { FALSE, FALSE, FALSE };
int enteringIndex = -1;
RECT doorRects[3] = {
    { 400, 350, 500, 450 },  // 문 1 (64x64)
    { 600, 350, 700, 450 },  // 문 2
    { 800, 350, 900, 450 }   // 문 3
};

GameState targetStages[DOOR_COUNT] = {
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

// 맵 초기화 
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

    barbarianCount = archerCount = arrowEffectCount = cannonShotCount = giantsCount = 0;

    InitializeBattle();

    lstrcpy(timeStr, TEXT("03:00"));

    cannons[0][0] = tower{ 625, 380, TRUE, 300, 300, 10 };
    cannons[0][1] = tower{ 670, 390, TRUE, 300, 300, 10 };
    cannons[0][2] = tower{ 655, 420, TRUE, 300, 300, 10 };
    cannons[0][3] = tower{ 610, 410, TRUE, 300, 300, 10 };
    cannons[1][0] = tower{ 686, 412, TRUE, 300, 300, 10 };
    cannons[1][1] = tower{ 640, 445, TRUE, 300, 300, 10 }; //한 칸 y =20 x =30
    cannons[1][2] = tower{ 583, 398, TRUE, 300, 300, 10 };
    cannons[1][3] = tower{ 645, 366, TRUE, 300, 300, 10 };
    mortars[1][0] = tower{ 658, 423, TRUE, 300, 300, 10,200 };
    mortars[1][1] = tower{ 613, 378, TRUE, 300, 300, 10,200 };
    cannons[2][0] = tower{ 686, 412, TRUE, 300, 300, 10 };
    cannons[2][1] = tower{ 640, 445, TRUE, 300, 300, 10 };
    cannons[2][2] = tower{ 583, 398, TRUE, 300, 300, 10 };
    cannons[2][3] = tower{ 645, 366, TRUE, 300, 300, 10 };
    mortars[2][0] = tower{ 658, 423, TRUE, 300, 300, 10,500 };
    mortars[2][1] = tower{ 613, 378, TRUE, 300, 300, 10,500 };
    infernos[2][0] = InfernoTower{ 659, 378, TRUE, 300, 300, 0, 0.1f, FALSE, -1, FALSE };
    infernos[2][1] = InfernoTower{ 610, 410, TRUE, 300, 300, 0, 0.1f, FALSE, -1, FALSE };
    villageking = tower{ 642, 392, TRUE, 600, 600 };

    for (int i = 0; i < MAX_EFFECTS; i++)
    {
        cannonShots[i] = shot{ 0 };
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
    for (int i = 0; i < 3; i++) naturaltowercount[i] = towercount[i];

    stagestart = FALSE;

    menusoundplay = FALSE;

    // 초기화시 타이머 설정
    GameFramework::SetSpeed(1.0);
    GameFramework::KillTimer(IDT_STAGE_TIMER);
    GameFramework::StartTimer(IDT_ANIM_TIMER, 200);
}

// 게임 리셋
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
    for (int i = 0; i < 3; i++) earn_star[i] = 0;
}

void AddArrowEffect(ArrowEffect effect) {
    if (arrowEffectCount >= arrowEffectCapacity) {
        arrowEffectCapacity += 100;
        arrowEffects = (ArrowEffect*)realloc(arrowEffects, sizeof(ArrowEffect) * arrowEffectCapacity);
    }
    arrowEffects[arrowEffectCount++] = effect;
}

// 화면 상태에 맞는 배경음으로 전환한다.
static void UpdateBgm(void)
{
    if (isLoading)
        return;

    if (gameState == STATE_MENU)
    {
        if (!menusoundplay)
        {
            PlaySound(0, 0, NULL);
            PlaySound(MAKEINTRESOURCE(IDR_WAVE2), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
            menusoundplay = TRUE;
        }
    }
    else if (gameState != STATE_SHOP && gameState != STATE_END)
    {
        if (!stagesoundplay)
        {
            PlaySound(0, 0, NULL);
            PlaySound(MAKEINTRESOURCE(IDR_WAVE3), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC | SND_LOOP);
            stagesoundplay = TRUE;
        }
    }
}
void UpdateUnlockStatus() {
    int star = earn_star[0] + earn_star[1] + earn_star[2];
    stage2Unlocked = unlockOverride || (star >= 3);
    stage3Unlocked = unlockOverride || (star >= 6);
    archerUnlocked = unlockOverride || (star >= 3);
    giantUnlocked = unlockOverride || (star >= 6);
}


// 틱마다 게임 타이머에 따라 일 처리
static void OnGameTimer(int timerId)
{
    if (gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3)
        if (CheckStageEnd(gameState - STATE_STAGE1)) {
            gameState = STATE_END;
        }

    if (timerId == IDT_STAGE_TIMER && gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3)
    {
        if (Stagetime > 0) {
            UpdateBattleTime();

            int minutes = Stagetime / 60;
            int seconds = Stagetime % 60;

            wsprintf(timeStr, TEXT("%02d:%02d"), minutes, seconds);
        }
        else {
            GameFramework::KillTimer(IDT_ANIM_TIMER);
            wsprintf(timeStr, TEXT("00:00"));
        }
    }

    if (timerId == IDT_LOADING_TIMER && isLoading) {
        loadingProgress += 5;
        if (loadingProgress >= 100) {
            isLoading = FALSE;
            gameState = STATE_MENU;
            GameFramework::KillTimer(IDT_LOADING_TIMER);
        }
    }

    else if (timerId == IDT_ANIM_TIMER && gameState == STATE_MENU)
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

    else if (timerId == IDT_ANIM_TIMER && gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3) {
        int stageIdx = gameState - STATE_STAGE1;

        UpdateTowerCooldowns(stageIdx);

        RemoveDeadUnits();

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

        UpdateUnits(stageIdx);

        UpdateArrows(stageIdx);

        UpdateCannonAttacks(stageIdx);

        UpdateCannonShots();

        UpdateMortarAttacks(stageIdx);

        UpdateMortarShots();

        InfernoAttack(stageIdx);

        UpdateInfernoEffects(stageIdx);
    }
}

// 지금은 솔로 게임 전체를 하나의 모드로 감싼다. 이후 로딩/메뉴/전투 등으로 나눈다.
class MainGameMode : public GameMode {
public:
    void OnTimer(int timerId) override { OnGameTimer(timerId); }
};

static MainGameMode mainMode;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
    HWND hWnd;
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

    GameFramework::StartTimer(IDT_LOADING_TIMER, 100, false);   // 로딩은 배속의 영향을 받지 않는다
    GameFramework::StartTimer(IDT_ANIM_TIMER, 200);

    return GameFramework::Run(hWnd, &mainMode);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {

    switch (iMessage) {
    case WM_CREATE:
        PlaySound(MAKEINTRESOURCE(IDR_WAVE1), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
        if (loadingProgress >= 100) {
            isLoading = FALSE;
            gameState = STATE_MENU;
            GameFramework::KillTimer(IDT_LOADING_TIMER);
            PlaySound(NULL, 0, 0);
        }
        Renderer_Init(g_hInst);
        InitGame(hWnd);
        //isLoading = FALSE; // 주석처리로 로딩 끄기 가능
        //gameState = STATE_MENU;
        shopButton = RECT{ 50, 800, 200, 860 };  // 좌측 하단 대략 위치
        break;

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam), y = HIWORD(lParam);

        if (gameState == STATE_SHOP) {
            if (PtInRect(&shopExitButton, POINT { x, y })) {
                gameState = STATE_MENU;
                InvalidateRect(hWnd, NULL, FALSE);
                return 0;
            }
            POINT pt = { x, y };

            RECT init_upgrade = { uis[3].x, 960 - uis[3].y - 150, uis[3].x + 150, 960 - uis[3].y - 100 };

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
                if (PtInRect(&doorRects[i], POINT { x, y })) {
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
                if (PtInRect(&stageButtons[i], POINT { x, y })) {
                    gameState = static_cast<GameState>(STATE_STAGE1 + i);
                    barbarianCount = 0;
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }

            if (PtInRect(&shopButton, POINT { x, y })) {
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
                        barbarians[barbarianCount++] = Unit{
                            x, y, 0, 7 + unitSpeed, {0, 0}, 100, 100, 10 + unitAtk, 25,
                            tx, ty, (tx != -1), targetIdx, 0,
                            0
                        };
                        mana -= 5;
                    }

                    // W 키 → 아처
                    else if (GetAsyncKeyState('W') & 0x8000 && archerCount < MAX_UNITS) {
                        if (!archerUnlocked) return 0;
                        archers[archerCount++] = Unit{
                            x, y, 0, 7 + unitSpeed, {0, 0}, 80, 80, 15 + unitAtk, 75,
                            tx, ty, (tx != -1), targetIdx, 0,
                            0
                        };
                        mana -= 5;
                    }
                }

                if (GetAsyncKeyState('E') & 0x8000 && giantsCount < MAX_UNITS && mana - 10 >= 0) {
                    if (!giantUnlocked) return 0;
                    giants[giantsCount++] = Unit{
                        x, y, 0, 4 + unitSpeed, {0, 0}, 500, 500, 25 + unitAtk, 25,
                        tx, ty, (tx != -1), targetIdx, 0,
                        0
                    };
                    mana -= 10;
                }

                if (!stagestart)
                {
                    GameFramework::StartTimer(IDT_STAGE_TIMER, 1000);
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
            GameFramework::SetSpeed(2.0);
            break;

        case ID_SPEED_SLOW:
            GameFramework::SetSpeed(0.5);
            break;

        case ID_SPEED_NORMAL:
            GameFramework::SetSpeed(1.0);
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

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        UpdateBgm();
        Renderer_Paint(hWnd, hDC);
        EndPaint(hWnd, &ps);
        break;
    }

    case WM_DESTROY:
        Renderer_Shutdown();
        PlaySound(0, 0, NULL);
        GameFramework::KillTimer(IDT_ANIM_TIMER);
        GameFramework::KillTimer(IDT_LOADING_TIMER);
        GameFramework::KillTimer(IDT_STAGE_TIMER);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hWnd, iMessage, wParam, lParam);
}
