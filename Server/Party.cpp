#include "Party.h"
#include <algorithm>

Party::Party(PartyId partyId, MapId mapId) : partyId_(partyId), mapId_(mapId)
{

}

bool Party::Has(PlayerId pid) const
{
    return std::find(members_.begin(), members_.end(), pid) != members_.end();
}

bool Party::AddMember(PlayerId pid)
{
    if (pid == kInvalidPlayerId) return false;
    if (status_ != PartyStatus::Waiting) return false;   // 전투가 시작되면 잠긴다
    if (IsFull()) return false;
    if (Has(pid)) return false;                          // 중복 참가 방지

    members_.push_back(pid);
    if (leaderId_ == kInvalidPlayerId)
        leaderId_ = pid;                                 // 첫 참가자가 방장
    return true;
}

bool Party::RemoveMember(PlayerId pid)
{
    auto it = std::find(members_.begin(), members_.end(), pid);
    if (it == members_.end()) return false;
    members_.erase(it);                                  // 순서를 유지하며 제거
    return true;
}

bool Party::CanStart(PlayerId pid) const
{
    return status_ == PartyStatus::Waiting
        && !members_.empty()                             // 1명이어도 시작 가능
        && members_.size() <= kMaxPartyMembers
        && pid == leaderId_
        && Has(pid);
}

PartyInfo Party::BuildPartyState(const NameLookup& nameOf) const
{
    PartyInfo info;
    info.partyId = partyId_;
    info.mapId = mapId_;
    info.leaderId = leaderId_;
    info.members.reserve(members_.size());
    for (PlayerId pid : members_) {
        PartyMemberInfoView m;
        m.playerId = pid;
        if (nameOf) m.name = nameOf(pid);
        info.members.push_back(std::move(m));
    }
    return info;
}

PlayerId Party::TransferLeader()
{
    leaderId_ = members_.empty() ? kInvalidPlayerId : members_.front();
    return leaderId_;
}

void Party::Reset()
{
    members_.clear();
    leaderId_ = kInvalidPlayerId;
    status_ = PartyStatus::Waiting;
    chatLog_.clear();           // 파티가 해체되면 채팅 내역도 함께 삭제
    chatThrottle_.clear();
}

ChatResult Party::AppendChat(PlayerId pid, const std::string& text, ChatClock::time_point now)
{
    if (!Has(pid)) return ChatResult::NotMember;
    if (text.empty() || text.size() > kMaxChatBytes) return ChatResult::InvalidText;

    ChatThrottle& throttle = chatThrottle_[pid];
    if (now < throttle.blockedUntil) return ChatResult::Blocked;   // 막힌 시간 안에는 모두 거절

    // 판정 시간보다 오래된 기록은 버림
    const std::chrono::milliseconds window(kChatBurstWindowMs);
    while (!throttle.recent.empty() && now - throttle.recent.front() >= window)
        throttle.recent.pop_front();

    // 판정 시간 안에 이미 한도만큼 보냈다면 이번 건이 한도 초과이므로 막고 이 건도 저장하지 않음
    if (throttle.recent.size() >= kChatBurstLimit) {
        throttle.blockedUntil = now + std::chrono::milliseconds(kChatBlockMs);
        throttle.recent.clear();
        return ChatResult::Blocked;
    }

    throttle.recent.push_back(now);

    ChatEntry entry;
    entry.senderId = pid;
    entry.text = text;
    chatLog_.push_back(std::move(entry));
    if (chatLog_.size() > kMaxChatHistory)
        chatLog_.erase(chatLog_.begin());   // 가장 오래된 내역부터 삭제

    return ChatResult::Ok;
}