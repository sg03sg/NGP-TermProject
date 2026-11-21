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
}