#include "BattleManager.h"
#include <iostream>

BattleCreateResult BattleManager::CreateBattle(PartyId partyId, const MapData& mapData, const std::vector<PlayerId>& members, BattleInstance*& outBattle)
{
    outBattle = nullptr;

    if (battles_.find(partyId) != battles_.end()) {
        return BattleCreateResult::AlreadyExists;
    }

    auto battle = std::make_unique<BattleInstance>(partyId);

    if (!battle->Initialize(mapData, members)) {
        std::cout << "[BattleManager] 전투 초기화 실패 (party " << partyId << ")" << std::endl;
        return BattleCreateResult::InitFailed;
    }

    // 초기화가 끝난 전투만 보관
    outBattle = battle.get();
    battles_.emplace(partyId, std::move(battle));

    return BattleCreateResult::Created;
}

BattleInstance* BattleManager::FindBattle(PartyId partyId)
{
    auto it = battles_.find(partyId);
    return it == battles_.end() ? nullptr : it->second.get();
}

bool BattleManager::RemoveBattle(PartyId partyId)
{
    return battles_.erase(partyId) > 0;
}

size_t BattleManager::RemoveBattlesWithoutParty(const std::function<bool(PartyId)>& partyExists)
{
    size_t removed = 0;

    for (auto it = battles_.begin(); it != battles_.end(); ) {
        if (partyExists(it->first)) {
            ++it;
            continue;
        }

        std::cout << "[BattleManager] 파티가 없는 전투 정리 (party " << it->first << ")" << std::endl;
        it = battles_.erase(it);
        ++removed;
    }

    return removed;
}

void BattleManager::Clear()
{
    battles_.clear();
}
