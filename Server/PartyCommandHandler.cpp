#include "PartyCommandHandler.h"
#include <iostream>

PartyCommandHandler::PartyCommandHandler(Context ctx) : ctx_(std::move(ctx))
{

}

void PartyCommandHandler::HandleJoinMap(const Command& cmd, PlayerId pid)
{
    JoinMap msg;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, msg, &header)) {
        std::cout << "[Party] JOIN_MAP 디코딩 실패 (Session ID: " << cmd.sessionId << ")" << std::endl;
        return;
    }

    const PartyResult r = ctx_.parties.JoinMap(pid, msg.mapId);
    if (!r.success) {
        // INVALID_STAGE, STAGE_LOCKED, PARTY_FULL, ALREADY_IN_PARTY
        SendRejected(pid, MessageType::JOIN_MAP, r.error, header.requestId);
        return;
    }

    // 새 참가자를 포함한 파티원 전원에게 같은 명단을 보낸 뒤, 뒤늦게 들어온 사람에게만 지금까지의 채팅 내역을 보낸다.
    // 내역의 보낸 사람 이름을 명단에서 찾으므로 명단이 먼저 도착해야 한다.
    BroadcastPartyState(r.partyId);
    SendChatHistory(pid, r.partyId);
    if (ctx_.onLobbyChanged) ctx_.onLobbyChanged();
}

void PartyCommandHandler::HandleLeaveParty(const Command& cmd, PlayerId pid)
{
    LeaveParty msg;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, msg, &header)) {
        std::cout << "[Party] LEAVE_PARTY 디코딩 실패 (Session ID: " << cmd.sessionId << ")" << std::endl;
        return;
    }

    // 실제 소속과 클라이언트가 보낸 partyId가 일치하는지 확인.
    const PartyId current = ctx_.parties.PartyOf(pid);
    if (current == kInvalidPartyId || current != msg.partyId) {
        SendRejected(pid, MessageType::LEAVE_PARTY, PartyError::NotInParty, header.requestId);
        return;
    }

    // 전투 중 이탈은 중단 투표(ABORT_VOTE_*)로 처리
    const Party* party = ctx_.parties.GetParty(current);
    if (party && party->Status() != PartyStatus::Waiting) {
        SendRejected(pid, MessageType::LEAVE_PARTY, PartyError::StageLocked, header.requestId);
        return;
    }

    // 명단 제거, 방장 이전, 빈 파티 삭제, 맵 점유 해제는 PartyManager가 한다.
    const PartyResult r = ctx_.parties.LeaveParty(pid);
    if (!r.success) {
        SendRejected(pid, MessageType::LEAVE_PARTY, r.error, header.requestId);
        return;
    }

    LeaveApproved ack;
    ack.partyId = r.partyId;
    if (ctx_.sendToPlayer) ctx_.sendToPlayer(pid, PacketCodec::Encode(ack, header.requestId));

    // 남은 파티원에게 갱신된 명단(새 방장 포함)을 보낸다. 마지막 사람이 나가 파티가 사라졌다면 보낼 대상이 없다.
    if (!r.disbanded) BroadcastPartyState(r.partyId);
    if (ctx_.onLobbyChanged) ctx_.onLobbyChanged();
}

void PartyCommandHandler::HandlePlayerLeft(PlayerId pid)
{
    if (!ctx_.parties.IsInParty(pid)) return;

    // (+) 전투 중(Running) 이탈은 중단 투표로 처리해야 한다. 지금은 대기 중과 같이 명단에서만 제거.
    const PartyResult r = ctx_.parties.LeaveParty(pid);
    if (!r.success) return;

    if (!r.disbanded) BroadcastPartyState(r.partyId);
    if (ctx_.onLobbyChanged) ctx_.onLobbyChanged();
}

void PartyCommandHandler::SendRejected(PlayerId pid, MessageType rejectedType, PartyError error, uint32_t requestId)
{
    RequestRejected msg;
    msg.rejectedType = static_cast<uint16_t>(rejectedType);
    msg.errorCode = static_cast<uint16_t>(error);   // PartyError의 숫자 값을 클라이언트와 공유해야 한다
    if (ctx_.sendToPlayer) ctx_.sendToPlayer(pid, PacketCodec::Encode(msg, requestId));
}

void PartyCommandHandler::BroadcastPartyState(PartyId partyId)
{
    const Party* party = ctx_.parties.GetParty(partyId);
    if (!party) return;

    const PartyInfo info = party->BuildPartyState(ctx_.nameOf);

    PartyState msg;
    msg.partyId = info.partyId;
    msg.mapId = info.mapId;
    msg.leaderId = info.leaderId;
    msg.memberCount = static_cast<uint8_t>(info.members.size());
    for (const PartyMemberInfoView& m : info.members) {
        PartyMemberInfo pm;
        pm.playerId = m.playerId;
        pm.name = m.name;          // 길이 필드는 Encode가 name.size()로 채운다
        msg.members.push_back(pm);
    }

    const Packet packet = PacketCodec::Encode(msg, 0);
    if (ctx_.sendToPlayers) ctx_.sendToPlayers(ctx_.parties.MembersOf(partyId), packet);
}

void PartyCommandHandler::SendChatHistory(PlayerId pid, PartyId partyId)
{
    const Party* party = ctx_.parties.GetParty(partyId);
    if (!party || !ctx_.sendToPlayer) return;

    // 오래된 것부터 한 건씩 전송, 공통 이벤트와 같이 requestId는 0
    for (const ChatEntry& entry : party->ChatLog()) {
        ChatMessage msg;
        msg.senderId = entry.senderId;
        msg.text = entry.text;
        msg.textLength = static_cast<uint16_t>(entry.text.size());
        ctx_.sendToPlayer(pid, PacketCodec::Encode(msg, 0));
    }
}