#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>

// 클라이언트 전용 드로잉 모듈. 게임 상태는 읽기만 한다.

// 비트맵 리소스를 한 번만 로드한다. WM_CREATE 에서 호출한다.
BOOL Renderer_Init(HINSTANCE hInst);

// 로드한 리소스를 해제한다. WM_DESTROY 에서 호출한다.
void Renderer_Shutdown(void);

// 현재 게임 상태에 맞는 화면을 그린다. WM_PAINT 에서 BeginPaint 로 얻은 hDC 를 넘긴다.
void Renderer_Paint(HWND hWnd, HDC hDC);

#endif
