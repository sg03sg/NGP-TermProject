#include "BattleInstance.h"
#include <iostream>

BattleInstance::BattleInstance(PartyId partyId) : partyId_(partyId)
{

}


bool BattleInstance::Initialize(const MapData& mapData, const std::vector<PlayerId>& members)
{
    if (members.empty()) {
        std::cerr << "[BattleInstance] Initialize failed: no members (party " << partyId_ << ")\n";
        return false;
    }

    mapDataId_ = mapData.id;
    players_.clear();
    units_.clear();
    towers_.clear();
    vote_ = AbortVote{};
    result_ = BattleResult{};
    tick_ = 0;
    remainingMs_ = kBattleTimeLimitMs;

    // 플레이어별 개인 마나
    players_.reserve(members.size());
    for (PlayerId pid : members) {
        PlayerState p;
        p.id = pid;
        p.maxMana = kMaxMana;
        p.mana = kInitialMana;
        players_.push_back(p);
    }

    // 맵 데이터 사본 복사
    towers_ = mapData.towers;
    for (Tower& t : towers_) {
        t.hp = t.maxHp;
        t.destroyed = false;
    }

    townHall_ = mapData.townHall;
    townHall_.hp = townHall_.maxHp;
    townHall_.destroyed = false;

    if (townHall_.maxHp == 0) {
        std::cerr << "[BattleInstance] Initialize failed: map " << int(mapData.id)
            << " has no valid town hall\n";
        return false;
    }
    return true;
}