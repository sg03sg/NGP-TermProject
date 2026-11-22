#include "PartyUI.h"
#include "LobbyUiCommon.h"
#include "../resource.h"

#pragma comment(lib, "msimg32.lib")

namespace {
    // 파티 창 이미지 크기와 화면 중앙 배치
    constexpr int kPanelWidth = 480;
    constexpr int kPanelHeight = 480;
    constexpr int kPanelLeft = (kScreenWidth - kPanelWidth) / 2;
    constexpr int kPanelTop = (kScreenHeight - kPanelHeight) / 2;

    // 이미지 안쪽 좌표
    constexpr int kStageTop = 62;
    constexpr int kStageHeight = 26;
    constexpr int kSlotLeft = 30;
    constexpr int kSlotWidth = 420;
    constexpr int kSlotTop = 100;
    constexpr int kSlotHeight = 52;
    constexpr int kSlotStep = 64;
    constexpr int kSlotNumberWidth = 40;   // 슬롯 왼쪽의 번호가 차지하는 폭
    constexpr int kMessageTop = 362;
    constexpr int kMessageHeight = 32;
    constexpr int kButtonTop = 410;
    constexpr int kButtonWidth = 140;
    constexpr int kButtonHeight = 46;
    constexpr int kStartLeft = 60;
    constexpr int kLeaveLeft = 280;
    constexpr int kStartOffMargin = 4;   // 회색 버튼 이미지가 시작 버튼보다 사방으로 더 크게 덮는 폭

    constexpr COLORREF kTransparentColor = RGB(255, 0, 255);

    HBITMAP LoadResourceBitmap(int resourceId)
    {
        return (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(resourceId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
    }

    // 마젠타를 투명색으로 두고 이미지를 화면 좌표에 그림
    void DrawTransparentBitmap(HDC hdc, HBITMAP bitmap, int x, int y)
    {
        if (bitmap == nullptr) return;

        BITMAP info;
        GetObject(bitmap, sizeof(info), &info);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
        TransparentBlt(hdc, x, y, info.bmWidth, info.bmHeight, memDC, 0, 0, info.bmWidth, info.bmHeight, kTransparentColor);
        SelectObject(memDC, oldBitmap);
        DeleteDC(memDC);
    }

    RECT ScreenRect(int left, int top, int width, int height)
    {
        return RECT{ kPanelLeft + left, kPanelTop + top, kPanelLeft + left + width, kPanelTop + top + height };
    }
}

PartyUI::PartyUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId) : state_(state), send_(std::move(send)), nextRequestId_(std::move(nextRequestId))
{

}

PartyUI::~PartyUI()
{
    if (panelBitmap_) DeleteObject(panelBitmap_);
    if (startOffBitmap_) DeleteObject(startOffBitmap_);
}

void PartyUI::LoadBitmaps()
{
    if (panelBitmap_ == nullptr)    panelBitmap_ = LoadResourceBitmap(IDB_PARTY_WINDOW);
    if (startOffBitmap_ == nullptr) startOffBitmap_ = LoadResourceBitmap(IDB_PARTY_START_OFF);
}

RECT PartyUI::StartButtonRect() { return ScreenRect(kStartLeft, kButtonTop, kButtonWidth, kButtonHeight); }
RECT PartyUI::LeaveButtonRect() { return ScreenRect(kLeaveLeft, kButtonTop, kButtonWidth, kButtonHeight); }

void PartyUI::Draw(HDC hdc, const PartyState& partyState)
{
    LoadBitmaps();
    DrawTransparentBitmap(hdc, panelBitmap_, kPanelLeft, kPanelTop);

    // 스테이지 정보는 제목 아래에 표시
    const RECT stageArea = ScreenRect(0, kStageTop, kPanelWidth, kStageHeight);
    DrawLabel(hdc, stageArea, L"스테이지 " + std::to_wstring(partyState.mapId), RGB(169, 187, 223), 20, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 정원만큼 슬롯을 순서대로 채우고 남는 슬롯은 빈자리로 표시
    for (int i = 0; i < kMaxPartyMembers; ++i) {
        const RECT slot = ScreenRect(kSlotLeft + kSlotNumberWidth, kSlotTop + i * kSlotStep, kSlotWidth - kSlotNumberWidth * 2, kSlotHeight);

        if (i >= (int)partyState.members.size()) {
            DrawLabel(hdc, slot, L"빈자리", RGB(125, 137, 166), 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            continue;
        }

        const PartyMemberInfo& member = partyState.members[i];
        std::wstring text = Utf8ToWide(member.name);
        if (member.playerId == partyState.leaderId) text += L"  [방장]";
        if (member.playerId == state_.MyPlayerId())  text += L"  (나)";

        DrawLabel(hdc, slot, text, RGB(255, 255, 255), 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (!message_.empty()) {
        const RECT messageArea = ScreenRect(0, kMessageTop, kPanelWidth, kMessageHeight);
        DrawLabel(hdc, messageArea, message_, RGB(255, 220, 120), 20, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 시작 버튼은 방장만 쓸 수 있으므로 방장이 아니면 회색 버튼으로 덮음
    if (!state_.IsPartyLeader()) {
        const RECT start = StartButtonRect();
        DrawTransparentBitmap(hdc, startOffBitmap_, start.left - kStartOffMargin, start.top - kStartOffMargin);
    }
}

bool PartyUI::OnStartClicked()
{
    if (state_.Screen() != ScreenState::PartyWaiting || !state_.InParty()) return false;
    if (!state_.IsPartyLeader()) {
        message_ = PartyErrorText(6);   // 방장만 시작 가능
        return false;
    }

    StartGame request;
    request.partyId = state_.Party().partyId;

    const PacketCodec::Packet packet = PacketCodec::Encode(request, nextRequestId_());
    if (packet.empty() || !send_(packet)) {
        message_ = L"서버로 요청을 보내지 못했습니다.";
        return false;
    }

    message_ = L"시작을 요청했습니다.";
    return true;
}

bool PartyUI::OnLeaveClicked()
{
    if (state_.Screen() != ScreenState::PartyWaiting || !state_.InParty()) return false;
    if (pendingLeaveId_ != 0) return false;   // 이전 퇴장 요청의 응답을 기다리는 중

    LeaveParty request;
    request.partyId = state_.Party().partyId;

    const uint32_t requestId = nextRequestId_();
    const PacketCodec::Packet packet = PacketCodec::Encode(request, requestId);
    if (packet.empty() || !send_(packet)) {
        message_ = L"서버로 요청을 보내지 못했습니다.";
        return false;
    }

    pendingLeaveId_ = requestId;
    message_ = L"퇴장 요청 중...";
    return true;
}

void PartyUI::OnLeaveApproved()
{
    pendingLeaveId_ = 0;
    message_.clear();
}

void PartyUI::OnRejected(uint32_t requestId, uint16_t errorCode)
{
    if (requestId == pendingLeaveId_) pendingLeaveId_ = 0;
    message_ = PartyErrorText(errorCode);
}

void PartyUI::Reset()
{
    pendingLeaveId_ = 0;
    message_.clear();
}