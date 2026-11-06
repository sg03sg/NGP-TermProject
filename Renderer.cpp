// 클라이언트 전용 드로잉 모듈. 게임 상태는 읽기만 하고 수정하지 않음
// 비트맵 리소스의 로드/해제와 화면 상태별 그리기를 담당한다.
#include "Renderer.h"
#include "RendererGlobals.h"
#include "resource.h"
#include <tchar.h>
#include <math.h>

#pragma comment(lib, "msimg32.lib")

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 940

// 비트맵 리소스
static HBITMAP  hRightWalkBmp[3], hWalkBmp[3], hgiantsbmp[3], hrightgiantbmp[3], hArcherBmp[3], hrightarcherbmp[3], hLogoBmp, hCannonBmp, hmortarbmp, hInfernoBmp, harrowBmp, hcannonBallBmp, hvillagekingBmp, hui0bmp, hui1bmp, hui2bmp, hui3bmp, hui4bmp, hui5bmp, hui6bmp, endvbmp, enddbmp;
static BITMAP bmpRightWalk[3], bmpWalk[3], bmpgiants[3], bmprightgiant[3], bmprightarcher[3], aWalk[3], bmpLogo, bmpCannon, bmpmortar, bmpInferno, bmpArrow, bmpCannonBall, bmpvillageking, bmpui0, bmpui1, bmpui2, bmpui3, bmpui4, bmpui5, bmpui6, bmpendv, bmpendd;
static HBITMAP hStageMap[3];
static BITMAP bmpStageMap[3];
static HBITMAP hDoorFrames[5], hMenuBg, hShopMap, hshopbababmp, hshoparcherbmp, hshopgiantbmp;
static BITMAP bmpDoorFrames[5], bmpMenuBg, bmpShopMap, bmpshopbaba, bmpshoparcher, bmpshopgiant;

static const TCHAR* tips[] = {
    _T("Tip: 전략을 잘 세워 스테이지를 클리어해 보세요."),
    _T("Tip: 클리어가 힘들다면 업그레이드를 활용해 보세요."),
    _T("Tip: 별 3개 도달 시 2스테이지 진입 및 아처 해금입니다."),
    _T("Tip: 별 6개 도달 시 3스테이지 진입 및 자이언트 해금입니다.")
};

// 리소스 로드 / 해제 ----------------------------------------------------------------------------------------------------------------------------------
BOOL Renderer_Init(HINSTANCE hInst) {
    hMenuBg = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP24), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hMenuBg) GetObject(hMenuBg, sizeof(BITMAP), &bmpMenuBg);

    for (int i = 0; i < 5; i++) {
        hDoorFrames[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP25 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hDoorFrames[i]) GetObject(hDoorFrames[i], sizeof(BITMAP), &bmpDoorFrames[i]);
    }

    hshoparcherbmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP32), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hshoparcherbmp) GetObject(hshoparcherbmp, sizeof(BITMAP), &bmpshoparcher);

    hshopbababmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP33), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hshopbababmp) GetObject(hshopbababmp, sizeof(BITMAP), &bmpshopbaba);

    hshopgiantbmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP34), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hshopgiantbmp) GetObject(hshopgiantbmp, sizeof(BITMAP), &bmpshopgiant);

    hShopMap = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP26), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hShopMap) GetObject(hShopMap, sizeof(BITMAP), &bmpShopMap);

    endvbmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP22), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (endvbmp) GetObject(endvbmp, sizeof(BITMAP), &bmpendv);

    enddbmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP21), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (enddbmp) GetObject(enddbmp, sizeof(BITMAP), &bmpendd);

    hui0bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP17), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui0bmp) GetObject(hui0bmp, sizeof(BITMAP), &bmpui0);

    hui1bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui1bmp) GetObject(hui1bmp, sizeof(BITMAP), &bmpui1);

    hui2bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP7), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui2bmp) GetObject(hui2bmp, sizeof(BITMAP), &bmpui2);

    hui3bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP16), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui3bmp) GetObject(hui3bmp, sizeof(BITMAP), &bmpui3);

    hui4bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP18), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui4bmp) GetObject(hui4bmp, sizeof(BITMAP), &bmpui4);

    hui5bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP19), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui5bmp) GetObject(hui5bmp, sizeof(BITMAP), &bmpui5);

    hui6bmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP20), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hui6bmp) GetObject(hui6bmp, sizeof(BITMAP), &bmpui6);

    hvillagekingBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP13), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hvillagekingBmp) GetObject(hvillagekingBmp, sizeof(BITMAP), &bmpvillageking);

    hLogoBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP41), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hLogoBmp) GetObject(hLogoBmp, sizeof(BITMAP), &bmpLogo);

    for (int i = 0; i < 3; i++) {
        hWalkBmp[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP2 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hWalkBmp[i]) GetObject(hWalkBmp[i], sizeof(BITMAP), &bmpWalk[i]);

        hgiantsbmp[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP14 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hgiantsbmp[i]) GetObject(hgiantsbmp[i], sizeof(BITMAP), &bmpgiants[i]);
    }
    for (int i = 0; i < 3; i++) {
        hRightWalkBmp[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP10 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hRightWalkBmp[i]) GetObject(hRightWalkBmp[i], sizeof(BITMAP), &bmpRightWalk[i]);;
    }

    for (int i = 0; i < 3; i++)
    {
        hArcherBmp[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP35 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hArcherBmp[i]) GetObject(hArcherBmp[i], sizeof(BITMAP), &aWalk[i]);

        hrightarcherbmp[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP38 + i), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hrightarcherbmp[i]) GetObject(hrightarcherbmp[i], sizeof(BITMAP), &bmprightarcher[i]);
    }

    harrowBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP8), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (harrowBmp) GetObject(harrowBmp, sizeof(BITMAP), &bmpArrow);

    hCannonBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP5), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hCannonBmp) GetObject(hCannonBmp, sizeof(BITMAP), &bmpCannon);

    hmortarbmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP23), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hmortarbmp) GetObject(hmortarbmp, sizeof(BITMAP), &bmpmortar);

    hInfernoBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP31), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hInfernoBmp) GetObject(hInfernoBmp, sizeof(BITMAP), &bmpInferno);

    hcannonBallBmp = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP9), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (hcannonBallBmp) GetObject(hcannonBallBmp, sizeof(BITMAP), &bmpCannonBall);

    for (int i = 0; i < 3; i++) {
        hStageMap[i] = static_cast<HBITMAP>(LoadImage(hInst, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
        if (hStageMap[i]) GetObject(hStageMap[i], sizeof(BITMAP), &bmpStageMap[i]);
    }


    return TRUE;
}

static void DeleteBmp(HBITMAP* h) {
    if (*h) {
        DeleteObject(*h);
        *h = NULL;
    }
}

static void DeleteBmpArray(HBITMAP* arr, int count) {
    for (int i = 0; i < count; i++)
        DeleteBmp(&arr[i]);
}

void Renderer_Shutdown(void) {
    DeleteBmpArray(hRightWalkBmp, 3);
    DeleteBmpArray(hWalkBmp, 3);
    DeleteBmpArray(hgiantsbmp, 3);
    DeleteBmpArray(hArcherBmp, 3);
    DeleteBmpArray(hrightarcherbmp, 3);
    DeleteBmpArray(hStageMap, 3);
    DeleteBmpArray(hDoorFrames, 5);
    DeleteBmp(&hLogoBmp);
    DeleteBmp(&hCannonBmp);
    DeleteBmp(&hmortarbmp);
    DeleteBmp(&hInfernoBmp);
    DeleteBmp(&harrowBmp);
    DeleteBmp(&hcannonBallBmp);
    DeleteBmp(&hvillagekingBmp);
    DeleteBmp(&hui0bmp);
    DeleteBmp(&hui1bmp);
    DeleteBmp(&hui2bmp);
    DeleteBmp(&hui3bmp);
    DeleteBmp(&hui4bmp);
    DeleteBmp(&hui5bmp);
    DeleteBmp(&hui6bmp);
    DeleteBmp(&endvbmp);
    DeleteBmp(&enddbmp);
    DeleteBmp(&hMenuBg);
    DeleteBmp(&hShopMap);
    DeleteBmp(&hshopbababmp);
    DeleteBmp(&hshoparcherbmp);
    DeleteBmp(&hshopgiantbmp);
}

// 화면 상태별 그리기 ----------------------------------------------------------------------------------------------------------------------------------
//박격포 탄
static void drawShot(HDC hdc, shot* p) {
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

static void DrawHealthBar(HDC hDC, int x, int y, int width, int height, int hp, int maxHp) {
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

static void DrawLoadingScreen(HDC hDC, HDC mDC, int width, int height) {
    // 배경
    RECT screenRect{ 0, 0, width, height };

    FillRect(mDC, &screenRect, CreateSolidBrush(RGB(20, 20, 20)));

    if (hLogoBmp) {
        HDC memDC = CreateCompatibleDC(hDC);
        SelectObject(memDC, hLogoBmp);
        StretchBlt(mDC, 0, 0, width, height, memDC, 0, 0, bmpLogo.bmWidth, bmpLogo.bmHeight, SRCCOPY);
        DeleteDC(memDC);
    }

    int bx = (width - 400) / 2;
    int by = height - 145;

    RECT barRect = { bx, by, bx + 400, by + 30 };
    FillRect(mDC, &barRect, CreateSolidBrush(RGB(60, 60, 60)));
	RECT frameRect = { bx + 2, by + 2, bx + 2 + (396 * loadingProgress / 100), by + 28 };
    FillRect(mDC, &frameRect, CreateSolidBrush(RGB(180, 0, 255)));

    SetBkMode(mDC, TRANSPARENT);
    SetTextColor(mDC, RGB(255, 255, 255));
	RECT textRect = { 0, by + 40, width, by + 80 };
    DrawText(mDC, tips[loadingProgress % 4], -1, &textRect, DT_CENTER);
}

static void DrawMenuScreen(HDC mDC, int width, int height) {
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
    if (barbarianEntering[0] == TRUE || barbarianEntering[1] == TRUE || barbarianEntering[2] == TRUE)
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
    TextOut(mDC, uis[3].x + 40, uis[3].y, starText, lstrlen(starText));
    SelectObject(mDC, hOldFont);
    DeleteObject(hFont);

}

static void DrawShopScreen(HDC mDC) {
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
        TransparentBlt(mDC, shopunit.left + 50, shopunit.top, shopunit.right, shopunit.bottom, memDC, 0, 0, bmpshopbaba.bmWidth, bmpshopbaba.bmHeight, shopbaba_backcol);

        SelectObject(memDC, hshoparcherbmp);
        shopbaba_backcol = GetPixel(memDC, 0, 1);
        TransparentBlt(mDC, shopunit.left + shopunit.right + 170, shopunit.top, shopunit.right, shopunit.bottom, memDC, 0, 0, bmpshoparcher.bmWidth, bmpshoparcher.bmHeight, shopbaba_backcol);

        SelectObject(memDC, hshopgiantbmp);
        shopbaba_backcol = GetPixel(memDC, 0, 1);
        TransparentBlt(mDC, shopunit.left + 2 * shopunit.right + 320, shopunit.top + 30, shopunit.right - 20, shopunit.bottom - 50, memDC, 0, 0, bmpshopgiant.bmWidth, bmpshopgiant.bmHeight, shopbaba_backcol);

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

static void DrawStageScreen(HDC mDC, HDC paintDC, int stageIdx, HWND hWnd) {
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
        COLORREF baba_back_color = GetPixel(paintDC, 0, 1);
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
        TransparentBlt(mDC, x, y, tileHeight * 1.3f, tileHeight * 1.3, paintDC, 0, 0,
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

    for (int i = 0; i < mortarCount[stageIdx]; i++) {
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
    TextOut(mDC, uis[1].x + 25, uis[1].y + uis[1].height - 5, _T("5"), _tcslen(_T("5")));
    MoveToEx(mDC, uis[1].x + 55, uis[1].y, NULL);
    LineTo(mDC, uis[1].x + 55, uis[1].y + uis[1].height);

    SelectObject(paintDC, hui2bmp);
    COLORREF  ui2_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[1].x + 60, uis[1].y, uis[1].length, uis[1].height, paintDC, 0, 0, bmpui2.bmWidth, bmpui2.bmHeight, ui2_back_color);
    TextOut(mDC, uis[1].x + 85, uis[1].y + uis[1].height - 5, _T("5"), _tcslen(_T("5")));
    MoveToEx(mDC, uis[1].x + 115, uis[1].y, NULL);
    LineTo(mDC, uis[1].x + 115, uis[1].y + uis[1].height);

    SelectObject(paintDC, hui3bmp);
    COLORREF  ui3_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[1].x + 120, uis[1].y, uis[1].length, uis[1].height, paintDC, 0, 0, bmpui3.bmWidth, bmpui3.bmHeight, ui3_back_color);
    const TCHAR* str = _T("10");
    TextOut(mDC, uis[1].x + 145, uis[1].y + uis[1].height - 5, str, _tcslen(str));

    int mana_length;
    int earn = earn_star[0] + earn_star[1] + earn_star[2];
    if (earn < 3)
        mana_length = uis[2].length;
    else if (earn >= 3 && earn < 6)
        mana_length = uis[2].length + 10;
    else if (earn >= 6)
        mana_length = uis[2].length + 20;

    SelectObject(paintDC, hui4bmp);
    COLORREF  ui4_back_color = GetPixel(paintDC, 0, 1);
    TransparentBlt(mDC, uis[2].x, uis[2].y, mana_length, uis[2].height, paintDC, 0, 0, bmpui4.bmWidth, bmpui4.bmHeight, ui4_back_color);


    HPEN pen_mana = (HPEN)GetStockObject(NULL_PEN);
    HBRUSH blue_mana = CreateSolidBrush(RGB(0, 0, 255));
    SelectObject(mDC, blue_mana);
    SelectObject(mDC, pen_mana);
    Rectangle(mDC, uis[2].x + 17, uis[2].y + 12, uis[2].x + 12 + mana, uis[2].y + 23);
    TCHAR buffer[30];
    wsprintf(buffer, L"%d / %d", mana, mana_length - 10);
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

// 인페르노 공격 그리기
static void DrawInfernoEffects(HDC mDC, int stageIdx) {
    for (int i = 0; i < infernoEffectCount; i++) {
        if (!infernoEffects[i].active) continue;

        int towerId = infernoEffects[i].sourceTowerId;
        if (!infernos[stageIdx][towerId].isAttacking) continue;

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
            continue;
        }

        HPEN hPen = CreatePen(PS_SOLID, 4, RGB(255, 253, 126));
        HPEN oldPen = static_cast<HPEN>(SelectObject(mDC, hPen));

        MoveToEx(mDC, (int)sx, (int)sy, NULL);
        LineTo(mDC, (int)tx, (int)ty);

        SelectObject(mDC, oldPen);
        DeleteObject(hPen);
    }
}

static void DRAWENDScreen(HWND hWnd, HDC mDC, HDC paintDC)
{
    RECT bkrect;
    GetClientRect(hWnd, &bkrect);
    if (victory)
    {
        SelectObject(paintDC, endvbmp);
        StretchBlt(mDC, 0, 0, bkrect.right, bkrect.bottom, paintDC, 0, 0, bmpendv.bmWidth, bmpendv.bmHeight, SRCCOPY);
        HFONT hFont = CreateFont(60, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, TEXT("맑은 고딕"));

        HFONT hOldFont = (HFONT)SelectObject(mDC, hFont);

        SetBkMode(mDC, TRANSPARENT);
        SetTextColor(mDC, RGB(0, 0, 0));
        RECT rect = { 300, 650, 800, 780 };
        DrawText(mDC, TEXT("게임승리!\n아무 곳이나 클릭하세요"), -1, &rect, DT_LEFT | DT_TOP | DT_WORDBREAK);

        SelectObject(mDC, hOldFont);
        DeleteObject(hFont);
    }

    else if (!victory && gameend)
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

static void DrawGrid(HDC hDC) {
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

static void Drawtowerrange(HDC hDC, int stageIdx) {
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(200, 130, 35));
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
    HBRUSH hbrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hDC, hbrush);


    for (int i = 0; i < cannonCount[stageIdx]; i++) {
        tower* c = &cannons[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x + tileWidth - 100, c->y + tileWidth - 100, c->x + tileWidth + 100, c->y + tileWidth + 100);
    }

    DeleteObject(hPen);
    hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    SelectObject(hDC, hPen);

    for (int i = 0; i < mortarCount[stageIdx]; i++) {
        tower* c = &mortars[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x + tileWidth - 200, c->y + tileWidth - 200, c->x + tileWidth + 200, c->y + tileWidth + 200);
    }

    DeleteObject(hPen);
    hPen = CreatePen(PS_SOLID, 2, RGB(242, 78, 4));
    SelectObject(hDC, hPen);

    for (int i = 0; i < infernoCount[stageIdx]; i++) {
        InfernoTower* c = &infernos[stageIdx][i];
        if (!c->active) continue;

        Ellipse(hDC, c->x + tileWidth - 50, c->y + tileWidth - 50, c->x + tileWidth + 50, c->y + tileWidth + 50);
    }

    SelectObject(hDC, hOldPen);
    DeleteObject(hPen);
}

static void Drawblock(HDC mDC, int stageIdx)
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

// WM_PAINT 에서 호출한다. 백버퍼에 그린 뒤 hDC 로 한 번에 복사한다.
void Renderer_Paint(HWND hWnd, HDC hDC) {
    HDC mDC = CreateCompatibleDC(hDC);
    HDC paintDC = CreateCompatibleDC(hDC);
    HBITMAP hBackBuf = CreateCompatibleBitmap(hDC, SCREEN_WIDTH, SCREEN_HEIGHT);
    HBITMAP hOldBuf = (HBITMAP)SelectObject(mDC, hBackBuf);

    if (isLoading)
        DrawLoadingScreen(hDC, mDC, SCREEN_WIDTH, SCREEN_HEIGHT);
    else if (gameState == STATE_MENU)
        DrawMenuScreen(mDC, SCREEN_WIDTH, SCREEN_HEIGHT);
    else if (gameState == STATE_SHOP)
        DrawShopScreen(mDC);
    else if (gameState == STATE_END)
        DRAWENDScreen(hWnd, mDC, paintDC);
    else {
        int stageIdx = gameState - STATE_STAGE1;
        DrawStageScreen(mDC, paintDC, stageIdx, hWnd);
        if (showGrid)
            DrawGrid(mDC);
        if (showrange)
            Drawtowerrange(mDC, stageIdx);
        if (showblock)
            Drawblock(mDC, stageIdx);
        DrawInfernoEffects(mDC, stageIdx);
    }

    BitBlt(hDC, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, mDC, 0, 0, SRCCOPY);

    SelectObject(mDC, hOldBuf);
    DeleteObject(hBackBuf);
    DeleteDC(mDC);
    DeleteDC(paintDC);
}
