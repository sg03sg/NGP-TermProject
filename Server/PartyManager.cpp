#include "PartyManager.h"

PartyResult PartyManager::JoinMap(PlayerId pid, MapId mapId)
{
    PartyResult r;
    r.mapId = mapId;

    if (!IsValidMap(mapId)) { r.error = PartyError::InvalidStage; return r; }
    if (playerToParty_.count(pid)) { r.error = PartyError::AlreadyInParty; return r; }

    auto mapIt = mapToParty_.find(mapId);
    if (mapIt == mapToParty_.end()) {
        // 새 파티를 만들고 첫 참가자가 방장
        const PartyId id = nextPartyId_;
        auto inserted = parties_.emplace(id, Party(id, mapId));
        if (!inserted.second || !inserted.first->second.AddMember(pid)) {
            parties_.erase(id);
            r.error = PartyError::InvalidStage;
            return r;
        }
        ++nextPartyId_;
        mapToParty_[mapId] = id;
        playerToParty_[pid] = id;
        r.success = true;
        r.partyId = id;
        r.createdNew = true;
        return r;
    }

    Party& party = parties_.at(mapIt->second);
    if (party.Status() != PartyStatus::Waiting) { r.error = PartyError::StageLocked; return r; }
    if (party.IsFull()) { r.error = PartyError::PartyFull;   return r; }
    if (!party.AddMember(pid)) { r.error = PartyError::AlreadyInParty; return r; }

    playerToParty_[pid] = party.Id();
    r.success = true;
    r.partyId = party.Id();
    return r;
}

bool PartyManager::IsMapOpen(MapId mapId) const
{
    if (!IsValidMap(mapId)) return false;
    auto mapIt = mapToParty_.find(mapId);
    if (mapIt == mapToParty_.end()) return true;            // 비어 있으면 모집 가능
    const Party& party = parties_.at(mapIt->second);
    return party.Status() == PartyStatus::Waiting && !party.IsFull();
}

PartyResult PartyManager::LeaveParty(PlayerId pid)
{
    PartyResult r;

    auto pIt = playerToParty_.find(pid);
    if (pIt == playerToParty_.end()) { r.error = PartyError::NotInParty; return r; }

    const PartyId partyId = pIt->second;
    auto partyIt = parties_.find(partyId);
    if (partyIt == parties_.end()) {                        // 소속 정보만 정리
        playerToParty_.erase(pIt);
        r.error = PartyError::NotInParty;
        return r;
    }

    Party& party = partyIt->second;
    const bool wasLeader = (party.Leader() == pid);
    r.partyId = partyId;
    r.mapId = party.GetMapId();

    party.RemoveMember(pid);
    playerToParty_.erase(pid);
    r.success = true;

    if (party.IsEmpty()) {                                  // 빈 파티 삭제와 맵 점유 해제
        EraseParty(partyId);
        r.disbanded = true;
        return r;
    }
    if (wasLeader) {                                        // 남은 첫 사람에게 방장 이전
        r.newLeader = party.TransferLeader();
        r.leaderChanged = true;
    }
    return r;
}

void PartyManager::ReleaseMap(MapId mapId)
{
    auto mapIt = mapToParty_.find(mapId);
    if (mapIt == mapToParty_.end()) return;
    EraseParty(mapIt->second);
}

void PartyManager::EraseParty(PartyId partyId)
{
    auto partyIt = parties_.find(partyId);
    if (partyIt == parties_.end()) return;

    for (PlayerId pid : partyIt->second.Members())          // 남은 참가자의 소속 정보 정리
        playerToParty_.erase(pid);
    mapToParty_.erase(partyIt->second.GetMapId());
    partyIt->second.Reset();
    parties_.erase(partyIt);
}

Party* PartyManager::GetParty(PartyId partyId)
{
    auto it = parties_.find(partyId);
    return it == parties_.end() ? nullptr : &it->second;
}

const Party* PartyManager::GetParty(PartyId partyId) const
{
    auto it = parties_.find(partyId);
    return it == parties_.end() ? nullptr : &it->second;
}

std::vector<PlayerId> PartyManager::MembersOf(PartyId partyId) const
{
    const Party* p = GetParty(partyId);
    return p ? p->Members() : std::vector<PlayerId>{};
}

PartyId PartyManager::PartyOf(PlayerId pid) const
{
    auto it = playerToParty_.find(pid);
    return it == playerToParty_.end() ? kInvalidPartyId : it->second;
}

std::vector<LobbyMapInfo> PartyManager::BuildLobbyMaps() const
{
    std::vector<LobbyMapInfo> maps;
    for (MapId id = 1; id <= stageCount_; ++id) {
        LobbyMapInfo m;
        m.mapId = id;
        m.capacity = kMaxPartyMembers;

        auto mapIt = mapToParty_.find(id);
        if (mapIt != mapToParty_.end()) {
            const Party& party = parties_.at(mapIt->second);
            m.memberCount = static_cast<uint8_t>(party.Size());
            if (party.Status() != PartyStatus::Waiting) m.status = MapStatus::InBattle;
            else if (party.IsFull())                    m.status = MapStatus::Full;
        }
        maps.push_back(m);
    }
    return maps;
}