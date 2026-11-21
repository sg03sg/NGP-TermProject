#include "PartyUI.h"
#include "LobbyUiCommon.h"

namespace {
    constexpr int kSlotLeft = 380;
    constexpr int kSlotWidth = 520;
    constexpr int kSlotTop = 220;
    constexpr int kSlotHeight = 70;
    constexpr int kSlotStep = 90;
}

PartyUI::PartyUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId) : state_(state), send_(std::move(send)), nextRequestId_(std::move(nextRequestId))
{

}

RECT PartyUI::StartButtonRect() { return RECT{ 430, 780, 610, 840 }; }
RECT PartyUI::LeaveButtonRect() { return RECT{ 670, 780, 850, 840 }; }

void PartyUI::Draw(HDC hdc, const PartyState& partyState)
{
    RECT title = { 0, 100, kScreenWidth, 180 };
    DrawLabel(hdc, title, L"파티 대기실 · 스테이지 " + std::to_wstring(partyState.mapId), RGB(255, 255, 255), 40, DT_CENTER | DT_VCENTER | DT_SINGLELINE, true);

    // 정원(4명)만큼 칸을 그리고, 비어 있으면 빈자리로 표시
    for (int i = 0; i < kMaxPartyMembers; ++i) {
        RECT slot = { kSlotLeft, kSlotTop + i * kSlotStep, kSlotLeft + kSlotWidth, kSlotTop + i * kSlotStep + kSlotHeight };
        const bool filled = i < (int)partyState.members.size();

        if (!filled) {
            DrawBox(hdc, slot, RGB(35, 35, 45), RGB(90, 90, 100));
            DrawLabel(hdc, slot, L"빈자리", RGB(120, 120, 130), 28, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            continue;
        }

        const PartyMemberInfo& member = partyState.members[i];
        std::wstring text = Utf8ToWide(member.name);
        if (member.playerId == partyState.leaderId) text += L"  [방장]";
        if (member.playerId == state_.MyPlayerId())  text += L"  (나)";

        DrawBox(hdc, slot, RGB(40, 70, 100), RGB(230, 230, 230));
        DrawLabel(hdc, slot, text, RGB(255, 255, 255), 30, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    if (!message_.empty()) {
        RECT messageArea = { 0, 700, kScreenWidth, 760 };
        DrawLabel(hdc, messageArea, message_, RGB(255, 220, 120), 28, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // 시작 버튼은 방장만 쓸 수 있으므로 아니면 회색으로
    const bool canStart = state_.IsPartyLeader();
    const RECT start = StartButtonRect();
    DrawBox(hdc, start, canStart ? RGB(40, 120, 70) : RGB(70, 70, 70), RGB(230, 230, 230));
    DrawLabel(hdc, start, L"시작", canStart ? RGB(255, 255, 255) : RGB(140, 140, 140), 30, DT_CENTER | DT_VCENTER | DT_SINGLELINE, true);

    const RECT leave = LeaveButtonRect();
    DrawBox(hdc, leave, RGB(130, 50, 50), RGB(230, 230, 230));
    DrawLabel(hdc, leave, L"나가기", RGB(255, 255, 255), 30, DT_CENTER | DT_VCENTER | DT_SINGLELINE, true);
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

bool PartyUI::HandleClick(int x, int y)
{
    const POINT point = { x, y };
    const RECT start = StartButtonRect();
    const RECT leave = LeaveButtonRect();

    if (PtInRect(&start, point)) { OnStartClicked(); return true; }
    if (PtInRect(&leave, point)) { OnLeaveClicked(); return true; }
    return false;
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