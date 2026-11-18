#include "GameWindow.h"
#include "../GameFramework.h"
#include "../Renderer.h"
#include "../RendererGlobals.h"

namespace {
	const TCHAR* WINDOW_CLASS = TEXT("ClientGameWindow");
	const TCHAR* WINDOW_TITLE = TEXT("Clash Of Clans");

	const int WINDOW_WIDTH = 1280;
	const int WINDOW_HEIGHT = 940;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message) {
		case WM_CREATE:
			Renderer_Init(((LPCREATESTRUCT)lParam)->hInstance);
			break;

		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hDC = BeginPaint(hWnd, &ps);
			Renderer_Paint(hWnd, hDC);
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
	if (!RegisterClassEx(&wc)) return NULL;

	HWND hWnd = CreateWindow(WINDOW_CLASS, WINDOW_TITLE, WS_OVERLAPPEDWINDOW,
		0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInst, NULL);
	if (hWnd == NULL) return NULL;

	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// 로딩은 배속의 영향을 받지 않음
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
