#include "PartyUI.h"
#include "LobbyUiCommon.h"
#include "../resource.h"

namespace {
    // 파티 창 이미지 크기와 화면 중앙 배치
    constexpr int kPanelWidth = 480;
    constexpr int kPanelHeight = 480;
    constexpr int kPanelLeft = (kScreenWidth - kPanelWidth) / 2;
    constexpr int kPanelTop = (kScreenHeight - kPanelHeight) / 2;

    // 이미지 안쪽 좌표
    constexpr int kStageTop = 77;      // 제목 글씨 아래와 첫 슬롯 위 사이의 좁은 틈
    constexpr int kStageHeight = 16;
    constexpr int kSlotNameLeft = 88;  // 슬롯 왼쪽의 번호 칸을 뺀 이름 표시 영역
    constexpr int kSlotNameWidth = 352;
    constexpr int kSlotTextHeight = 44;
    constexpr int kSlotCenterY[kMaxPartyMembers] = { 125, 195, 266, 338 };   // 슬롯마다 세로 중심이 조금씩 다름
    constexpr int kMessageTop = 372;
    constexpr int kMessageHeight = 24;
    constexpr int kButtonTop = 396;
    constexpr int kButtonHeight = 63;
    constexpr int kStartLeft = 58;
    constexpr int kStartWidth = 152;
    constexpr int kLeaveLeft = 276;
    constexpr int kLeaveWidth = 146;
    constexpr int kStartOffMargin = 4;   // 회색 버튼 이미지가 시작 버튼보다 사방으로 더 크게 덮는 폭

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

RECT PartyUI::PanelRect() { return RECT{ kPanelLeft, kPanelTop, kPanelLeft + kPanelWidth, kPanelTop + kPanelHeight }; }
RECT PartyUI::StartButtonRect() { return ScreenRect(kStartLeft, kButtonTop, kStartWidth, kButtonHeight); }
RECT PartyUI::LeaveButtonRect() { return ScreenRect(kLeaveLeft, kButtonTop, kLeaveWidth, kButtonHeight); }

void PartyUI::Draw(HDC hdc, const PartyState& partyState)
{
    LoadBitmaps();
    DrawTransparentBitmap(hdc, panelBitmap_, kPanelLeft, kPanelTop);

    // 스테이지 정보는 제목 아래에 표시
    const RECT stageArea = ScreenRect(0, kStageTop, kPanelWidth, kStageHeight);
    DrawLabel(hdc, stageArea, L"스테이지 " + std::to_wstring(partyState.mapId), RGB(169, 187, 223), 15, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 정원만큼 슬롯을 순서대로 채우고 남는 슬롯은 빈자리로 표시
    for (int i = 0; i < kMaxPartyMembers; ++i) {
        const RECT slot = ScreenRect(kSlotNameLeft, kSlotCenterY[i] - kSlotTextHeight / 2, kSlotNameWidth, kSlotTextHeight);

        if (i >= (int)partyState.members.size()) {
            DrawLabel(hdc, slot, L"빈자리", RGB(150, 145, 135), 22, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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
        DrawLabel(hdc, messageArea, message_, RGB(255, 220, 120), 18, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
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