#include "GameWindow.h"
#include "../GameFramework.h"
#include "../Renderer.h"
#include "../RendererGlobals.h"
#include "LobbyScreen.h"
#include "InputController.h"
#include "LobbyUiCommon.h"
#include <climits>
#include <cstdio>

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

			const bool menuOverlay = lobby != nullptr && !isLoading && gameState == STATE_MENU;
			const bool battleOverlay = input != nullptr && !isLoading && gameState >= STATE_STAGE1 && gameState <= STATE_STAGE3;

			if (menuOverlay || battleOverlay) {
				// 기존 화면을 백버퍼에 그린 뒤 메뉴의 파티 대기창이나 전투의 병종 선택 표시를 얹어 한 번에 복사
				HDC memDC = CreateCompatibleDC(hDC);
				HBITMAP backBuffer = CreateCompatibleBitmap(hDC, kScreenWidth, kScreenHeight);
				HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, backBuffer);

				Renderer_Paint(hWnd, memDC);
				if (menuOverlay) lobby->PaintOverlay(memDC);
				if (battleOverlay) input->PaintOverlay(memDC);
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
	Renderer_SetBattleStars(0);

	// 이전 전투의 구조물, 병력, 효과 비우기
	gameBattle.ClearUnits();
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
		info.structureId = structure.structureId;
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
				inferno.structureId = structure.structureId;
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

void GameWindow_Invalidate()
{
	if (mainWindow) InvalidateRect(mainWindow, NULL, FALSE);
}

void GameWindow_ApplyUnitCreated(const UnitCreated& message)
{
	// 패킷의 ID, 좌표, HP는 uint32_t이고 Unit은 int로 저장한다.
	if (message.unitId == 0 || message.unitId > INT_MAX ||
		message.ownerId == 0 ||
		message.unitType > static_cast<uint8_t>(UnitType::Giant) ||
		message.x > INT_MAX || message.y > INT_MAX ||
		message.maxHp == 0 || message.maxHp > INT_MAX ||
		message.hp > message.maxHp) {
		return;
	}

	const int unitId = static_cast<int>(message.unitId);
	if (gameBattle.FindUnit(unitId) != nullptr) {
		return;
	}

	const UnitType type = static_cast<UnitType>(message.unitType);
	UnitStats stats = GetUnitStats(type);
	stats.maxHp = static_cast<int>(message.maxHp);

	BattleUnit entry{
		message.ownerId,
		Unit(unitId, type,
			 static_cast<int>(message.x),
			 static_cast<int>(message.y),
			 stats)
	};

	// Unit 생성자는 현재 HP를 최대 HP로 초기화한다.
	entry.unit.TakeDamage(static_cast<int>(message.maxHp - message.hp));

	// 기존 화면 목록의 병종별 순서와 같은 병종 내 소환 순서를 유지한다.
	auto& units = gameBattle.GetUnits();
	auto position = units.begin();
	while (position != units.end() && position->unit.GetType() <= type) 
	{
		++position;
	}

	const auto inserted = units.insert(position, entry);
	std::printf("[화면 병력 등록] 서버 ID=%u 저장 ID=%d 좌표=(%d, %d)\n", message.unitId, inserted->unit.GetId(), inserted->unit.x, inserted->unit.y);
	GameWindow_Invalidate();
}

void GameWindow_ApplyBattleState(const BattleState& message)
{
	for (const UnitStateInfo& state : message.units) {
		if (state.unitId == 0 || state.unitId > INT_MAX ||
			state.x > INT_MAX || state.y > INT_MAX) {
			continue;
		}

		Unit* unit = gameBattle.FindUnit(static_cast<int>(state.unitId));
		if (unit == nullptr) {
			continue;
		}

		const int newX = static_cast<int>(state.x);
		const int newY = static_cast<int>(state.y);
		const bool moved = (unit->x != newX || unit->y != newY);

		if (newX != unit->x) {
			unit->facingRight = (newX > unit->x);
		}

		if (moved) {
			// 바바리안·아처는 20틱, 자이언트는 40틱마다 다음 걷기 그림을 표시한다.
			const uint32_t ticksPerFrame =
				(unit->GetType() == UnitType::Giant) ? 40u : 20u;

			unit->frameIndex = static_cast<int>((message.serverTick / ticksPerFrame) % 3u);
		}
		else {
			unit->frameIndex = 0;
		}

		unit->x = newX;
		unit->y = newY;
	}

	const int stageIdx = static_cast<int>(gameState) - static_cast<int>(STATE_STAGE1);
	if (stageIdx >= 0 && stageIdx < 3) {
		// 1. 서버가 보낸 모든 건물의 HP와 active를 먼저 갱신한다.
		for (const StructureStateInfo& state : message.structures) {
			if (state.structureId == 0 || state.maxHp == 0 ||
				state.maxHp > INT_MAX || state.hp > state.maxHp) {
				continue;
			}

			auto applyHp = [&](auto& building) {
				if (building.structureId != state.structureId) return;

				building.hp = static_cast<int>(state.hp);
				building.maxHp = static_cast<int>(state.maxHp);
				building.active = (state.hp > 0);
				};

			applyHp(villageking);

			for (int i = 0; i < cannonCount[stageIdx]; ++i)
				applyHp(cannons[stageIdx][i]);

			for (int i = 0; i < mortarCount[stageIdx]; ++i)
				applyHp(mortars[stageIdx][i]);

			for (int i = 0; i < infernoCount[stageIdx]; ++i)
				applyHp(infernos[stageIdx][i]);
		}

		int remainingTowers = 0;

		for (int i = 0; i < cannonCount[stageIdx]; ++i) {
			if (cannons[stageIdx][i].active) ++remainingTowers;
		}

		for (int i = 0; i < mortarCount[stageIdx]; ++i) {
			if (mortars[stageIdx][i].active) ++remainingTowers;
		}

		for (int i = 0; i < infernoCount[stageIdx]; ++i) {
			if (infernos[stageIdx][i].active) ++remainingTowers;
		}

		towercount[stageIdx] = remainingTowers;
	}

	GameWindow_Invalidate();
}

void GameWindow_ApplyBattleProgress(const BattleProgress& progress)
{
	const int seconds = progress.remainingSeconds;
	wsprintf(timeStr, TEXT("%02d:%02d"), seconds / 60, seconds % 60);

	Renderer_SetBattleStars(progress.stars);
	GameWindow_Invalidate();
}

void GameWindow_ApplyManaState(const ManaState& state)
{
	mana = state.mana;
	GameWindow_Invalidate();
}