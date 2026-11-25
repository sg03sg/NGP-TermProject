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

    // 파티 명단에서 보낸 사람의 이름을 찾음, 이미 나간 사람처럼 명단에 없으면 번호로 표시
    std::string SenderName(const ClientState& state, uint32_t senderId)
    {
        for (const PartyMemberInfo& member : state.Party().members)
            if (member.playerId == senderId) return member.name;
        return WideToUtf8(L"플레이어 ") + std::to_string(senderId);
    }

    // 채팅 전송이 거절되었을 때 채팅창에 보여줄 알림
    std::string ChatRejectText(uint16_t errorCode)
    {
        switch (errorCode) {
        case 5:  return WideToUtf8(L"참가 중인 파티가 없어 채팅을 보낼 수 없습니다.");
        case 7:  return WideToUtf8(L"도배 방지를 위해 잠시 채팅을 보낼 수 없습니다. 5초 뒤에 다시 보내세요.");
        default: return WideToUtf8(L"채팅을 보내지 못했습니다.");
        }
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

        if (rejected.rejectedType == static_cast<uint16_t>(MessageType::CHAT_SEND)) {
            ChatEntry notice;   // 보낸 사람이 0이면 채팅창에 알림 줄로 표시
            notice.text = ChatRejectText(rejected.errorCode);

            // 막힌 동안 반복해서 시도해도 같은 알림이 줄줄이 쌓이지 않도록 바로 앞 줄이 같은 알림이면 추가하지 않음
            const std::vector<ChatEntry>& log = state_.ChatLog();
            const bool repeated = !log.empty() && log.back().senderId == 0 && log.back().text == notice.text;
            if (!repeated) state_.AppendChat(notice);
        }
        else if (rejected.rejectedType == static_cast<uint16_t>(MessageType::JOIN_MAP))
            mapUI_.OnRejected(header.requestId, rejected.errorCode);
        else if (rejected.rejectedType == static_cast<uint16_t>(MessageType::LEAVE_PARTY) ||
            rejected.rejectedType == static_cast<uint16_t>(MessageType::START_GAME))
            partyUI_.OnRejected(header.requestId, rejected.errorCode);
        return true;
    }

    case MessageType::CHAT_MESSAGE: {
        ChatMessage chat;
        if (!PacketCodec::Decode(packet, chat)) {
            printf("[오류] CHAT_MESSAGE 해석 실패\n");
            return true;
        }

        ChatEntry entry;
        entry.senderId = chat.senderId;
        entry.name = SenderName(state_, chat.senderId);
        entry.text = chat.text;
        state_.AppendChat(entry);
        printf("[수신] CHAT_MESSAGE: 보낸 사람 %u, 본문 %zu바이트\n", chat.senderId, chat.text.size());
        return true;
    }

    default:
        return false;
    }
}

bool LobbyScreen::OnMapClicked(uint8_t mapId)
{
    const bool sent = mapUI_.OnMapClicked(mapId);
    Invalidate();
    return sent;
}

bool LobbyScreen::OnStartClicked()
{
    const bool sent = partyUI_.OnStartClicked();
    Invalidate();
    return sent;
}

bool LobbyScreen::OnLeaveClicked()
{
    const bool sent = partyUI_.OnLeaveClicked();
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