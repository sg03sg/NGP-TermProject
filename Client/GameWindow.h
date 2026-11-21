#pragma once

#include <windows.h>

// 게임 창 생성 후 로딩 타이머 시작
// 화면은 Renderer 모듈이 그리고, 창은 메시지 루프를 가진 GameFramework::Run에서 갱신
HWND GameWindow_Create(HINSTANCE hInst);

// GameFramework 타이머 틱마다 호출
void GameWindow_OnTimer(int timerId);

int GameWindow_TakeClickedMap();
