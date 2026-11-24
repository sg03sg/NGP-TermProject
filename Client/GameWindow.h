#pragma once

#include <windows.h>
#include <cstdint>
#include "../Message.h"

class LobbyScreen;
class InputController;

// 게임 창 생성 후 로딩 타이머 시작
// 화면은 Renderer 모듈이 그리고, 창은 메시지 루프를 가진 GameFramework::Run에서 갱신
HWND GameWindow_Create(HINSTANCE hInst);

// GameFramework 타이머 틱마다 호출
void GameWindow_OnTimer(int timerId);

// 파티 대기창을 게임 화면 위에 얹고 클릭을 넘겨줄 대상 등록
void GameWindow_SetLobby(LobbyScreen* lobby);

// 창의 마우스 클릭을 넘겨줄 대상 등록
void GameWindow_SetInput(InputController* input);

// BATTLE_INIT 내용으로 구조물·마나·시간을 채우고 해당 스테이지의 전투 화면으로 전환
void GameWindow_EnterBattle(const BattleInit& message, uint32_t myPlayerId);

// 창을 다시 그리도록 요청
void GameWindow_Invalidate();

// 서버가 알린 병력 생성(UNIT_CREATED)을 전투 화면의 병력에 반영
void GameWindow_ApplyUnitCreated(const UnitCreated& message);

// 서버가 알린 내 마나(MANA_STATE)를 전투 화면에 반영
void GameWindow_ApplyManaState(const ManaState& state);

void GameWindow_ApplyBattleState(const BattleState& message);

void GameWindow_ApplyBattleProgress(const BattleProgress& progress);