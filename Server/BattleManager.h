#pragma once
#include <functional>
#include <map>
#include <memory>
#include <vector>
#include "BattleInstance.h"

// 전투 생성 결과
enum class BattleCreateResult {
    Created,         // 생성하고 보관함
    AlreadyExists,   // 이 파티의 전투가 이미 있음
    InitFailed       // 맵 데이터나 인원이 잘못돼 초기화하지 못함
};


class BattleManager {
private:
    std::map<PartyId, std::unique_ptr<BattleInstance>> battles_;

public:
    BattleManager() = default;

    // 전투를 만들어 Initialize까지 마친 뒤 보관. 성공하면 outBattle에 만든 전투를 돌려준다.
    BattleCreateResult CreateBattle(PartyId partyId, const MapData& mapData, const std::vector<PlayerId>& members, BattleInstance*& outBattle);

    BattleInstance* FindBattle(PartyId partyId);

    // 보관 중인 전투를 지움.
    bool RemoveBattle(PartyId partyId);

    // 파티가 사라진 전투(모두 나가거나 연결이 끊긴 경우)를 지움. 지운 개수를 돌려준다.
    size_t RemoveBattlesWithoutParty(const std::function<bool(PartyId)>& partyExists);

    void Clear();

    // 게임 루프가 전투를 순회하며 갱신·방송할 때 씀
    std::map<PartyId, std::unique_ptr<BattleInstance>>& Battles() { return battles_; }
    const std::map<PartyId, std::unique_ptr<BattleInstance>>& Battles() const { return battles_; }
};
