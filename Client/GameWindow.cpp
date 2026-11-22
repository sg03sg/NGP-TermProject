#include "GameWindow.h"
#include "../GameFramework.h"
#include "../Renderer.h"
#include "../RendererGlobals.h"
#include "LobbyScreen.h"
#include "InputController.h"
#include "LobbyUiCommon.h"

namespace {
	const TCHAR* WINDOW_CLASS = TEXT("ClientGameWindow");
	const TCHAR* WINDOW_TITLE = TEXT("Clash Of Clans");

	const int WINDOW_WIDTH = 1280;
	const int WINDOW_HEIGHT = 940;

	HWND mainWindow = NULL;
	LobbyScreen* lobby = nullptr;
	InputController* input = nullptr;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message) {
		case WM_CREATE:
			Renderer_Init(((LPCREATESTRUCT)lParam)->hInstance);
			break;
		
		case WM_LBUTTONDOWN:
			// 클릭 위치 판별과 화면별 분배는 InputController가 담당
			if (input != nullptr) input->OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
			return 0;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);

			if (lobby != nullptr && !isLoading && gameState == STATE_MENU) {
				// 기존 메뉴 화면을 백버퍼에 그린 뒤 파티 대기창을 얹어 한 번에 복사
				HDC memDC = CreateCompatibleDC(hDC);
				HBITMAP backBuffer = CreateCompatibleBitmap(hDC, kScreenWidth, kScreenHeight);
				HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, backBuffer);

				Renderer_Paint(hWnd, memDC);
				lobby->PaintOverlay(memDC);
				BitBlt(hDC, 0, 0, kScreenWidth, kScreenHeight, memDC, 0, 0, SRCCOPY);

				SelectObject(memDC, oldBitmap);
				DeleteObject(backBuffer);
				DeleteDC(memDC);
			}
			else {
				Renderer_Paint(hWnd, hDC);
			}

			EndPaint(hWnd, &ps);
			break;
		}

		case WM_DESTROY:
			Renderer_Shutdown();
			GameFramework::KillTimer(IDT_LOADING_TIMER);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
}

HWND GameWindow_Create(HINSTANCE hInst)
{
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = WINDOW_CLASS;

	if (!RegisterClassEx(&wc)) {
		return NULL;
	}

	HWND hWnd = CreateWindow(
		WINDOW_CLASS,
		WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		0, 0,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL,
		hInst,
		NULL);

	if (hWnd == NULL) {
		return NULL;
	}

	mainWindow = hWnd;

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	GameFramework::StartTimer(IDT_LOADING_TIMER, 100, false);

	return hWnd;
}

void GameWindow_OnTimer(int timerId)
{
	if (timerId == IDT_LOADING_TIMER && isLoading) {
		loadingProgress += 5;

		if (loadingProgress >= 100) {
			isLoading = FALSE;
			gameState = STATE_MENU;

			GameFramework::KillTimer(IDT_LOADING_TIMER);
		}
	}
}

void GameWindow_SetLobby(LobbyScreen* lobbyScreen)
{
	lobby = lobbyScreen;
}

void GameWindow_SetInput(InputController* inputController)
{
	input = inputController;
}

void GameWindow_EnterBattle(const BattleInit& message, uint32_t myPlayerId)
{
	const int stageIdx = static_cast<int>(message.mapId) - 1;
	if (stageIdx < 0 || stageIdx >= 3) return;

	// 이전 전투의 구조물과 효과 비우기
	cannonCount[stageIdx] = 0;
	mortarCount[stageIdx] = 0;
	infernoCount[stageIdx] = 0;
	villageking = tower{};
	cannonShotCount = 0;
	arrowEffectCount = 0;
	infernoEffectCount = 0;

	// 서버가 보낸 구조물을 종류별 배열에 채움. 종류 번호는 서버의 StructureKind와 같음
	for (const StructureInitInfo& structure : message.structures) {
		tower info = {};
		info.x = static_cast<int>(structure.x);
		info.y = static_cast<int>(structure.y);
		info.active = TRUE;
		info.hp = static_cast<int>(structure.hp);
		info.maxHp = static_cast<int>(structure.maxHp);

		switch (structure.type) {
		case 0:
			villageking = info;
			break;
		case 1:
			if (cannonCount[stageIdx] < MAX_TOWER) cannons[stageIdx][cannonCount[stageIdx]++] = info;
			break;
		case 2:
			if (mortarCount[stageIdx] < MAX_TOWER) mortars[stageIdx][mortarCount[stageIdx]++] = info;
			break;
		case 3:
			if (infernoCount[stageIdx] < MAX_TOWER) {
				InfernoTower inferno = {};
				inferno.x = info.x;
				inferno.y = info.y;
				inferno.active = TRUE;
				inferno.hp = info.hp;
				inferno.maxHp = info.maxHp;
				inferno.targetId = -1;
				infernos[stageIdx][infernoCount[stageIdx]++] = inferno;
			}
			break;
		}
	}

	// 별 표시 기준이 되는 방어 타워 수
	towercount[stageIdx] = cannonCount[stageIdx] + mortarCount[stageIdx] + infernoCount[stageIdx];
	naturaltowercount[stageIdx] = towercount[stageIdx];

	// 내 마나와 남은 시간
	for (const PlayerInitInfo& player : message.players) {
		if (player.playerId == myPlayerId) mana = player.mana;
	}
	wsprintf(timeStr, TEXT("%02d:%02d"), message.remainingSeconds / 60, message.remainingSeconds % 60);

	gameState = static_cast<GameState>(STATE_STAGE1 + stageIdx);
	if (mainWindow) InvalidateRect(mainWindow, NULL, FALSE);
}
