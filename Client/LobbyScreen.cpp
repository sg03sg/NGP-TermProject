#include "LobbyScreen.h"
#include "LobbyUiCommon.h"
#include "../Protocol.h"
#include <cstdio>

namespace {
    uint16_t MessageTypeOf(const PacketCodec::Packet& packet)
    {
        if (packet.size() < PACKET_HEADER_SIZE) return 0;
        return (uint16_t)((packet[4] << 8) | packet[5]);
    }
}

LobbyScreen::LobbyScreen(MapUI::SendFn send, MapUI::RequestIdFn nextRequestId) : state_(), mapUI_(state_, send, nextRequestId), partyUI_(state_, send, nextRequestId)
{

}

void LobbyScreen::Invalidate() const
{
    if (hWnd_) InvalidateRect(hWnd_, NULL, FALSE);
}

void LobbyScreen::OnWelcome(uint32_t playerId)
{
    state_.SetMyPlayerId(playerId);
    Invalidate();
}

bool LobbyScreen::IsActive() const
{
    return state_.Screen() == ScreenState::Lobby || state_.Screen() == ScreenState::PartyWaiting;
}

std::vector<LobbyMapInfo> LobbyScreen::MapList() const
{
    // 서버가 로비 현황을 보내기 전에는 클라이언트가 가진 기본 목록(스테이지 3개)을 보여준다.
    return state_.LobbyMaps().empty() ? MakeDefaultLobbyMaps() : state_.LobbyMaps();
}

bool LobbyScreen::HandlePacket(const PacketCodec::Packet& packet)
{
    const bool handled = HandlePacketImpl(packet);
    if (handled) Invalidate();
    return handled;
}

bool LobbyScreen::HandlePacketImpl(const PacketCodec::Packet& packet)
{
    switch (static_cast<MessageType>(MessageTypeOf(packet))) {
    case MessageType::PARTY_STATE: {
        PartyState party;
        if (!PacketCodec::Decode(packet, party)) {
            printf("[오류] PARTY_STATE 해석 실패\n");
            return true;
        }
        state_.ApplyPartyState(party);
        mapUI_.OnJoined();
        printf("[수신] PARTY_STATE: 파티 %u, 스테이지 %u, 방장 %u, 참가자 %u명\n", party.partyId, party.mapId, party.leaderId, (unsigned)party.members.size());
        return true;
    }

    case MessageType::LEAVE_APPROVED: {
        LeaveApproved approved;
        if (!PacketCodec::Decode(packet, approved)) {
            printf("[오류] LEAVE_APPROVED 해석 실패\n");
            return true;
        }
        state_.Clear();
        partyUI_.OnLeaveApproved();
        mapUI_.Reset();
        printf("[수신] LEAVE_APPROVED: 파티 %u에서 나왔습니다.\n", approved.partyId);
        return true;
    }

    case MessageType::REQUEST_REJECT: {
        RequestRejected rejected;
        PacketHeader header;
        if (!PacketCodec::Decode(packet, rejected, &header)) {
            printf("[오류] REQUEST_REJECT 해석 실패\n");
            return true;
        }
        printf("[수신] REQUEST_REJECT: 요청 종류 %u 거절, 사유 %u\n", rejected.rejectedType, rejected.errorCode);

        if (rejected.rejectedType == static_cast<uint16_t>(MessageType::JOIN_MAP))
            mapUI_.OnRejected(header.requestId, rejected.errorCode);
        else if (rejected.rejectedType == static_cast<uint16_t>(MessageType::LEAVE_PARTY) ||
            rejected.rejectedType == static_cast<uint16_t>(MessageType::START_GAME))
            partyUI_.OnRejected(header.requestId, rejected.errorCode);
        return true;
    }

    default:
        return false;
    }
}

bool LobbyScreen::OnLButtonDown(int x, int y)
{
    // 맵 선택은 게임 화면의 문 클릭이 OnMapClicked로 넘어오므로 여기서는 파티 대기창만 처리
    if (state_.Screen() != ScreenState::PartyWaiting) return false;

    const bool onButton = partyUI_.HandleClick(x, y);
    if (onButton) Invalidate();
    return onButton;
}

bool LobbyScreen::OnMapClicked(uint8_t mapId)
{
    const bool sent = mapUI_.OnMapClicked(mapId);
    Invalidate();
    return sent;
}

void LobbyScreen::OnBattleInit(const BattleInit& message)
{
    state_.ApplyBattleInit(message);
    mapUI_.Reset();
    partyUI_.Reset();
    Invalidate();
}

bool LobbyScreen::IsPartyVisible() const
{
    return state_.Screen() == ScreenState::PartyWaiting;
}

void LobbyScreen::PaintOverlay(HDC hdc)
{
    if (state_.Screen() == ScreenState::PartyWaiting)
        partyUI_.Draw(hdc, state_.Party());   // 서버가 보낸 최신 파티 정보로 매번 다시 그림
    else if (state_.Screen() == ScreenState::Lobby)
        mapUI_.DrawMessage(hdc);
}

void LobbyScreen::Paint(HWND hWnd, HDC hdc)
{
    RECT client;
    GetClientRect(hWnd, &client);
    const int width = client.right > 0 ? client.right : kScreenWidth;
    const int height = client.bottom > 0 ? client.bottom : kScreenHeight;

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP backBuffer = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, backBuffer);

    RECT whole = { 0, 0, width, height };
    DrawBox(memDC, whole, RGB(20, 20, 30), RGB(20, 20, 30));

    if (state_.Screen() == ScreenState::PartyWaiting)
        partyUI_.Draw(memDC, state_.Party());
    else
        mapUI_.Draw(memDC, MapList());

    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBitmap);
    DeleteObject(backBuffer);
    DeleteDC(memDC);
}