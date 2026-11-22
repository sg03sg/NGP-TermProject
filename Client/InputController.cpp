#include "InputController.h"
#include "../RendererGlobals.h"

InputController::InputController(LobbyScreen& lobby) : lobby_(lobby)
{

}

void InputController::OnLButtonDown(int x, int y)
{
    // 로딩 중에는 모든 클릭 무시
    if (isLoading) return;

    const POINT point = { x, y };

    switch (lobby_.State().Screen()) {
    case ScreenState::Lobby:
        OnLobbyClick(point);
        break;
    case ScreenState::PartyWaiting:
        OnPartyClick(point);
        break;
    default:
        break;   // 전투 화면 클릭은 이후 구현
    }
}

void InputController::OnLobbyClick(const POINT& point)
{
    // 문이 그려지는 메뉴 화면에서만 맵 선택
    if (gameState != STATE_MENU) return;

    for (int i = 0; i < DOOR_COUNT; ++i) {
        if (PtInRect(&doorRects[i], point)) {
            lobby_.OnMapClicked(static_cast<uint8_t>(i + 1));   // 문 번호가 곧 스테이지 번호
            return;
        }
    }
}

void InputController::OnPartyClick(const POINT& point)
{
    // 파티 대기창이 떠 있는 동안 창 밖 클릭은 뒤의 화면으로 전달하지 않고 무시
    const RECT start = PartyUI::StartButtonRect();
    const RECT leave = PartyUI::LeaveButtonRect();

    if (PtInRect(&start, point)) lobby_.OnStartClicked();
    else if (PtInRect(&leave, point)) lobby_.OnLeaveClicked();
}
