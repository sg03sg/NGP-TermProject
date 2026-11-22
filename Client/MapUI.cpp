#include "MapUI.h"
#include "LobbyUiCommon.h"

namespace {
    constexpr int kButtonWidth = 300;
    constexpr int kButtonHeight = 220;
    constexpr int kButtonGap = 60;
    constexpr int kButtonTop = 330;

    const wchar_t* StatusText(MapStatus status)
    {
        switch (status)
        {
        case MapStatus::Open:     return L"모집 중";

        case MapStatus::Full:     return L"가득 참";

        case MapStatus::InBattle: return L"공략 중";
        }

        return L"";
    }
}

MapUI::MapUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId) : state_(state), send_(std::move(send)), nextRequestId_(std::move(nextRequestId))
{

}

RECT MapUI::ButtonRect(size_t index, size_t count) const
{
    RECT rect = { 0, 0, 0, 0 };
    if (count == 0) return rect;

    const int total = (int)count * kButtonWidth + (int)(count - 1) * kButtonGap;
    const int left = (kScreenWidth - total) / 2 + (int)index * (kButtonWidth + kButtonGap);
    rect.left = left;
    rect.top = kButtonTop;
    rect.right = left + kButtonWidth;
    rect.bottom = kButtonTop + kButtonHeight;
    return rect;
}

void MapUI::Draw(HDC hdc, const std::vector<LobbyMapInfo>& mapList)
{
    RECT title = { 0, 180, kScreenWidth, 260 };
    DrawLabel(hdc, title, L"스테이지 선택", RGB(255, 255, 255), 44, DT_CENTER | DT_VCENTER | DT_SINGLELINE, true);

    for (size_t i = 0; i < mapList.size(); ++i) {
        const LobbyMapInfo& map = mapList[i];
        const RECT button = ButtonRect(i, mapList.size());

        COLORREF fill = RGB(40, 90, 60);                                 // 모집 중
        if (map.status == MapStatus::Full)          fill = RGB(90, 90, 90);
        else if (map.status == MapStatus::InBattle) fill = RGB(120, 45, 45);
        DrawBox(hdc, button, fill, RGB(230, 230, 230));

        RECT nameArea = { button.left, button.top + 40, button.right, button.top + 110 };
        DrawLabel(hdc, nameArea, L"스테이지 " + std::to_wstring(map.mapId), RGB(255, 255, 255), 34, DT_CENTER | DT_VCENTER | DT_SINGLELINE, true);

        RECT statusArea = { button.left, button.top + 120, button.right, button.top + 180 };
        const std::wstring status = std::wstring(StatusText(map.status)) + L"  " + std::to_wstring(map.memberCount) + L" / " + std::to_wstring(map.capacity);
        DrawLabel(hdc, statusArea, status, RGB(255, 255, 255), 26, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    DrawMessage(hdc);
}

void MapUI::DrawMessage(HDC hdc)
{
    if (message_.empty()) return;

    // 배경 그림 위에서도 읽히도록 어두운 바탕 위에 표시
    RECT messageArea = { 340, 600, 940, 660 };
    DrawBox(hdc, messageArea, RGB(20, 20, 30), RGB(200, 200, 200));
    DrawLabel(hdc, messageArea, message_, RGB(255, 220, 120), 28, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

bool MapUI::OnMapClicked(uint8_t mapId)
{
    if (state_.Screen() == ScreenState::Connecting) {
        message_ = L"아직 서버에 접속되지 않았습니다.";
        return false;
    }
    if (state_.Screen() != ScreenState::Lobby) return false;
    if (state_.InParty()) {
        message_ = PartyErrorText(4);   // 이미 파티에 참가 중
        return false;
    }
    if (pendingRequestId_ != 0) return false;   // 이전 요청의 응답을 기다리는 중

    JoinMap request;
    request.mapId = mapId;

    const uint32_t requestId = nextRequestId_();
    const PacketCodec::Packet packet = PacketCodec::Encode(request, requestId);
    if (packet.empty() || !send_(packet)) {
        message_ = L"서버로 요청을 보내지 못했습니다.";
        return false;
    }

    pendingRequestId_ = requestId;
    message_ = L"참가 요청 중...";
    return true;
}

void MapUI::OnJoined()
{
    pendingRequestId_ = 0;
    message_.clear();
}

void MapUI::OnRejected(uint32_t requestId, uint16_t errorCode)
{
    if (requestId != pendingRequestId_) return;   // 이 화면이 보낸 요청에 대한 거절만 처리
    pendingRequestId_ = 0;
    message_ = PartyErrorText(errorCode);
}

void MapUI::Reset()
{
    pendingRequestId_ = 0;
    message_.clear();
}