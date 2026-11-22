#pragma once
#include "LobbyScreen.h"

// 마우스 클릭이 어느 화면의 어느 요소에서 일어났는지 한 곳에서 판별하고 해당 UI의 처리 함수를 호출
// 현재는 로비의 맵 선택과 파티 대기창의 버튼만 처리하며 전투 화면 클릭은 이후 추가
class InputController {
public:
    explicit InputController(LobbyScreen& lobby);

    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    void OnLButtonDown(int x, int y);   // 창의 왼쪽 클릭 좌표를 받아 현재 화면 단계에 맞게 분배

private:
    void OnLobbyClick(const POINT& point);   // 로비 화면에서 문 위 클릭이면 해당 맵 선택
    void OnPartyClick(const POINT& point);   // 파티 대기창에서 시작·나가기 버튼 위 클릭이면 해당 요청

    LobbyScreen& lobby_;
};
