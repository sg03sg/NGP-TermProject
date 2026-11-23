#include "InputController.h"
#include "GameWindow.h"
#include "../RendererGlobals.h"

namespace {
    // 전투 화면 하단 병종 UI의 슬롯, Renderer가 uis[1]을 기준으로 60픽셀 간격으로 그리는 위치와 같음
    constexpr int kUnitSlotCount = 3;
    constexpr int kUnitSlotStep = 60;
    constexpr int kUnitSlotWidth[kUnitSlotCount] = { 55, 55, 70 };

    // 구조물 한 칸의 화면상 크기
    constexpr int kStructureSize = 64;

    RECT UnitSlotRect(int index)
    {
        const int left = uis[1].x + kUnitSlotStep * index;
        return RECT{ left, uis[1].y, left + kUnitSlotWidth[index], uis[1].y + uis[1].height };
    }

    RECT UnitPanelRect()
    {
        return RECT{ uis[0].x, uis[0].y, uis[0].x + uis[0].length, uis[0].y + uis[0].height };
    }

    bool IsOnTower(const tower& target, const POINT& point)
    {
        if (!target.active) return false;
        const RECT area = { target.x, target.y, target.x + kStructureSize, target.y + kStructureSize };
        return PtInRect(&area, point) != FALSE;
    }

    // 마을회관이나 방어 타워 위의 클릭인지
    bool IsOnStructure(const POINT& point)
    {
        if (IsOnTower(villageking, point)) return true;

        const int stageIdx = gameState - STATE_STAGE1;
        if (stageIdx < 0 || stageIdx >= 3) return false;

        for (int i = 0; i < cannonCount[stageIdx]; ++i)
            if (IsOnTower(cannons[stageIdx][i], point)) return true;
        for (int i = 0; i < mortarCount[stageIdx]; ++i)
            if (IsOnTower(mortars[stageIdx][i], point)) return true;
        for (int i = 0; i < infernoCount[stageIdx]; ++i) {
            const InfernoTower& inferno = infernos[stageIdx][i];
            if (!inferno.active) continue;
            const RECT area = { inferno.x, inferno.y, inferno.x + kStructureSize, inferno.y + kStructureSize };
            if (PtInRect(&area, point)) return true;
        }
        return false;
    }
}

InputController::InputController(LobbyScreen& lobby, ChatUI& chat, SendFn send, RequestIdFn nextRequestId) : lobby_(lobby), chat_(chat), send_(std::move(send)), nextRequestId_(std::move(nextRequestId))
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
    case ScreenState::Battle:
        OnBattleClick(point);
        break;
    default:
        break;
    }
}

bool InputController::IsChatInputActive() const
{
    return !isLoading && lobby_.State().Screen() == ScreenState::PartyWaiting && chat_.IsOpen();
}

void InputController::OnChar(wchar_t ch)
{
    // 파티 대기창에서 채팅창이 열려 있을 때만 키 입력을 채팅으로 전달
    if (!IsChatInputActive()) return;

    if (ch == L'\r') chat_.OnEnterKey();
    else chat_.OnChar(ch);
}

void InputController::OnKeyDown(WPARAM key)
{
    if (!IsChatInputActive()) return;
    chat_.OnKeyDown(key);
}

void InputController::OnImeComposition(const std::wstring& result, const std::wstring& composing)
{
    if (!IsChatInputActive()) return;
    chat_.OnComposition(result, composing);
}

void InputController::SelectUnit(UnitType type)
{
    selectedType_ = type;
    GameWindow_Invalidate();
}

void InputController::PaintOverlay(HDC hdc)
{
    if (lobby_.State().Screen() != ScreenState::Battle) return;

    // 선택된 병종 슬롯에 노란 테두리 표시
    const RECT slot = UnitSlotRect(static_cast<int>(selectedType_));
    HBRUSH brush = CreateSolidBrush(RGB(255, 220, 0));
    for (int i = 0; i < 3; ++i) {
        const RECT frame = { slot.left - i, slot.top - i, slot.right + i, slot.bottom + i };
        FrameRect(hdc, &frame, brush);
    }
    DeleteObject(brush);
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
    // 채팅 아이콘은 채팅창을 열고 닫음
    const RECT chatIcon = ChatUI::ToggleButtonRect();
    if (PtInRect(&chatIcon, point)) {
        chat_.Toggle();
        return;
    }

    // 파티 대기창이 떠 있는 동안 창 밖 클릭은 뒤의 화면으로 전달하지 않고 무시
    const RECT start = PartyUI::StartButtonRect();
    const RECT leave = PartyUI::LeaveButtonRect();

    if (PtInRect(&start, point)) lobby_.OnStartClicked();
    else if (PtInRect(&leave, point)) lobby_.OnLeaveClicked();
}

void InputController::OnBattleClick(const POINT& point)
{
    if (gameState < STATE_STAGE1 || gameState > STATE_STAGE3) return;

    // 하단 병종 UI 위 클릭은 병종 선택이며 UI의 나머지 부분은 전장으로 보지 않고 무시
    const RECT panel = UnitPanelRect();
    if (PtInRect(&panel, point)) {
        for (int i = 0; i < kUnitSlotCount; ++i) {
            const RECT slot = UnitSlotRect(i);
            if (PtInRect(&slot, point)) {
                SelectUnit(static_cast<UnitType>(i));
                return;
            }
        }
        return;
    }

    // 구조물 위에는 소환하지 않음
    if (IsOnStructure(point)) return;

    // 전장 클릭은 선택된 병종의 소환 요청으로 서버에 전송, 소환 성공 여부는 서버의 응답으로 반영
    PacketCodec::Packet packet;
    if (BuildSpawnRequest(selectedType_, point, packet) && send_(packet))
        printf("[송신] SPAWN_REQUEST type=%d position=(%ld, %ld)\n", static_cast<int>(selectedType_), point.x, point.y);
}

bool InputController::BuildSpawnRequest(UnitType type, const POINT& pos, PacketCodec::Packet& packet)
{
    // 화면 좌표가 곧 전장 좌표
    if (pos.x < 0 || pos.y < 0) return false;

    SpawnRequest request;
    request.unitType = static_cast<uint8_t>(type);
    request.x = static_cast<uint32_t>(pos.x);
    request.y = static_cast<uint32_t>(pos.y);

    packet = PacketCodec::Encode(request, nextRequestId_());
    return !packet.empty();
}
