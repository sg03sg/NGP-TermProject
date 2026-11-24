#include "BattleInstance.h"
#include <algorithm>
#include <climits>
#include <iostream>
#include <cmath>
#include <limits>

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
    ClearUnits();
    towers_.clear();
    vote_ = AbortVote{};
    //result_ = BattleResult{};
    result_ = BattleOutcome{};
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

Result BattleInstance::TrySpawnUnit(
    PlayerId pid, UnitType t, Position pos)
{
    Result result;

    // 1. 종료된 전투에서는 소환할 수 없다.
    if (result_.finished) {
        result.error = ErrorCode::NotInBattle;
        return result;
    }

    // 2. 이 전투에 참가한 플레이어인지 확인한다.
    auto player = std::find_if(
        players_.begin(),
        players_.end(),
        [pid](const PlayerState& state) {
            return state.id == pid;
        }
    );

    if (player == players_.end()) {
        result.error = ErrorCode::NotInBattle;
        return result;
    }

    // 3. 사용할 수 있는 병종인지 확인한다.
    if (t < UnitType::Barbarian || t > UnitType::Giant) {
        result.error = ErrorCode::InvalidUnit;
        return result;
    }

    // 4. 음수 좌표를 거부한다.
    // 맵 경계와 구조물 겹침 검사는 맵 기준을 맞춘 뒤 추가한다.
    if (pos.x < 0 || pos.y < 0) {
        result.error = ErrorCode::InvalidPosition;
        return result;
    }

    // 5. 병종별 최대 개수와 ID 발급 한도를 확인한다.
    if (CountUnits(t) >= MAX_UNITS || nextUnitId > INT_MAX) {
        result.error = ErrorCode::UnitLimit;
        return result;
    }

    // 6. 서버가 병종의 기본 능력치와 마나 비용을 결정한다.
    const UnitStats& stats = GetUnitStats(t);

    int manaCost = 5;

    if (t == UnitType::Giant) {
        manaCost = 10;
    }

    if (player->mana < manaCost) {
        result.error = ErrorCode::NotEnoughMana;
        return result;
    }

    // 7. 새 병력을 만든다.
    const int unitId = static_cast<int>(nextUnitId);

    BattleUnit entry{
        pid,
        Unit(unitId, t, pos.x, pos.y, stats)
    };

    // 기존 병종별 정렬과 같은 병종 내 소환 순서를 유지한다.
    auto position = units.begin();

    while (position != units.end() &&
           position->unit.GetType() <= t) {
        ++position;
    }

    // 목록에 등록한다. 등록 성공 후 ID와 마나를 변경한다.
    units.insert(position, entry);

    ++nextUnitId;
    player->mana -= manaCost;

    // 8. 생성 결과를 반환한다.
    result.success = true;
    result.error = ErrorCode::None;
    result.unitId = static_cast<uint32_t>(unitId);

    return result;
}

Unit* BattleInstance::FindUnit(int id)
{
    for (BattleUnit& entry : units) {
        if (entry.unit.GetId() == id) {
            return &entry.unit;
        }
    }

    return nullptr;
}

int BattleInstance::CountUnits(UnitType type) const
{
    int count = 0;

    for (const BattleUnit& entry : units) {
        if (entry.unit.GetType() == type) {
            ++count;
        }
    }

    return count;
}

void BattleInstance::ClearUnits()
{
    units.clear();
}

void BattleInstance::ClearUnits(UnitType type)
{
    units.erase(
        remove_if(units.begin(), units.end(),
            [type](const BattleUnit& entry) {
                return entry.unit.GetType() == type;
            }),
        units.end()
    );
}

void BattleInstance::RemoveDeadEntities()
{
    units.erase(
        remove_if(units.begin(), units.end(),
            [](const BattleUnit& entry) {
                return entry.unit.IsDead();
            }),
        units.end()
    );
}

std::vector<BattleUnit>& BattleInstance::GetUnits()
{
    return units;
}

const std::vector<BattleUnit>& BattleInstance::GetUnits() const
{
    return units;
}

BattleInit BattleInstance::BuildBattleInit() const
{
    BattleInit message;
    message.partyId = partyId_;
    message.mapId = mapDataId_;
    message.remainingSeconds =
        static_cast<uint16_t>(remainingMs_ / 1000);

    for (const PlayerState& player : players_) {
        message.players.push_back({
            player.id,
            player.mana,
            player.maxMana
            });
    }

    message.playerCount =
        static_cast<uint8_t>(message.players.size());

    // 마을회관을 먼저 담고 이어서 방어타워
    StructureInitInfo townHall;
    townHall.structureId = townHall_.id;
    townHall.type = static_cast<uint8_t>(StructureKind::TownHall);
    townHall.x = static_cast<uint32_t>(townHall_.pos.x);
    townHall.y = static_cast<uint32_t>(townHall_.pos.y);
    townHall.hp = townHall_.hp;
    townHall.maxHp = townHall_.maxHp;
    message.structures.push_back(townHall);

    // 타워 종류는 맵 데이터의 type 값을 그대로 사용
    for (const Tower& tower : towers_) {
        StructureInitInfo structure;
        structure.structureId = tower.id;
        structure.type = tower.type;
        structure.x = static_cast<uint32_t>(tower.pos.x);
        structure.y = static_cast<uint32_t>(tower.pos.y);
        structure.hp = tower.hp;
        structure.maxHp = tower.maxHp;
        message.structures.push_back(structure);
    }

    message.structureCount =
        static_cast<uint16_t>(message.structures.size());

    return message;
}

const PlayerState* BattleInstance::FindPlayer(PlayerId pid) const
{
    for (const PlayerState& state : players_) {
        if (state.id == pid) {
            return &state;
        }
    }

    return nullptr;
}

void BattleInstance::Update(uint32_t dtMs)
{
    if (dtMs == 0 || result_.finished || players_.empty()) {
        return;
    }

    const double dtSeconds =
        static_cast<double>(dtMs) / 1000.0;

    // 기존 이동 로직은 200ms마다 speed만큼 이동했다.
    constexpr double oldUpdateSeconds = 0.2;

    for (BattleUnit& entry : units) 
    {
        Unit& unit = entry.unit;

        if (unit.IsDead()) { continue; }

        bool foundTarget = false;

        int targetX = 0;
        int targetY = 0;
        int targetIndex = -1;
        // BattleLogic::UpdateUnit에서 사용하던 논리 중심 (타일 31x22, 구조물 64x64).
        constexpr int unitCenterOffsetX = 31 / 2;
        constexpr int unitCenterOffsetY = 22 / 2;
        constexpr int structureCenterOffset = 32;
        const double unitCenterX = entry.preciseX + unitCenterOffsetX;
        const double unitCenterY = entry.preciseY + unitCenterOffsetY;
        double targetDx = 0.0;
        double targetDy = 0.0;

        double nearestDistance =
            std::numeric_limits<double>::infinity();

        // 후보 구조물 중 가장 가까운 곳을 선택한다.
        auto considerTarget = [&](const Position& position, int index)
            {
                const double dx =
                    static_cast<double>(position.x) + structureCenterOffset - unitCenterX;

                const double dy =
                    static_cast<double>(position.y) + structureCenterOffset - unitCenterY;

                const double distance = std::hypot(dx, dy);

                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    targetDx = dx;
                    targetDy = dy;

                    targetX = position.x;
                    targetY = position.y;
                    targetIndex = index;

                    foundTarget = true;
                }
            };

        // 방어타워 후보
        for (size_t i = 0; i < towers_.size(); ++i) 
        {
            const Tower& tower = towers_[i];

            if (tower.destroyed || tower.hp == 0) { continue; }

            considerTarget(tower.pos, static_cast<int>(i));
        }

        // 마을회관 후보
        if (!townHall_.destroyed && townHall_.hp > 0) 
        {
            considerTarget(townHall_.pos, -1);
        }

        if (!foundTarget) 
        {
            unit.hasTarget = FALSE;
            continue;
        }

        unit.SetTarget(targetX, targetY, targetIndex);

        const double attackRange =
            unit.range > 0 ? static_cast<double>(unit.range) : 0.0;

        // 사거리 경계의 부동소수점 오차까지 포함해 도착을 판정한다.
        if (nearestDistance <= attackRange + 1e-6)
        {
            if (unit.GetType() != UnitType::Barbarian) {
                continue;
            }

            // 서버에서는 남은 공격 대기 시간을 밀리초로 관리한다.
            if (unit.attackCooldown > 0) {
                const uint32_t remainingMs =
                    static_cast<uint32_t>(unit.attackCooldown);

                if (dtMs < remainingMs) {
                    unit.attackCooldown -= static_cast<int>(dtMs);
                    continue;
                }
            }

            // targetIndex는 위에서 가장 가까운 구조물을 고를 때 정한 값이다.
            // -1은 마을회관, 0 이상은 towers_의 인덱스다.
            auto hit = [&](auto& structure) {
                const uint32_t damage = static_cast<uint32_t>(unit.atk);
                structure.hp = damage >= structure.hp ? 0u : structure.hp - damage;
                structure.destroyed = (structure.hp == 0);
                };

            if (targetIndex == -1) {
                hit(townHall_);
            }
            else {
                hit(towers_[targetIndex]);
            }

            // 기존 코드는 200ms 갱신을 10번 쉬고 다음 갱신에 다시 공격한다.
            unit.attackCooldown = 2200;
            continue;
        }

        if (unit.speed <= 0)
        {
            continue;
        }

        // 기존 speed를 초당 이동 속도로 환산
        const double pixelsPerSecond = static_cast<double>(unit.speed) / oldUpdateSeconds;

        const double possibleMovement = pixelsPerSecond * dtSeconds;

        // 목표의 사거리 경계를 넘어가지 않도록 이동량 제한
        const double distanceToRange = nearestDistance - attackRange;

        const double movement = possibleMovement < distanceToRange ? possibleMovement : distanceToRange;

        // 방향을 정규화한 뒤 이동량을 곱한다.
        entry.preciseX += targetDx / nearestDistance * movement;
        entry.preciseY += targetDy / nearestDistance * movement;

        // 현재 Unit과 패킷은 정수 좌표를 사용한다.
        unit.x = static_cast<int>(std::lround(entry.preciseX));
        unit.y = static_cast<int>(std::lround(entry.preciseY));
    }
    // 서버 틱마다 남은 시간 감소 
    if (dtMs >= remainingMs_)
    {
        remainingMs_ = 0u;
    }
    else {
        remainingMs_ -= dtMs;
    }

    ++tick_;
}

BattleState BattleInstance::BuildBattleState() const
{
    BattleState message;

    message.partyId = partyId_;
    message.serverTick = tick_;

    for (const BattleUnit& entry : units) {
        const Unit& unit = entry.unit;

        if (unit.IsDead()) {
            continue;
        }

        UnitStateInfo state;
        state.unitId = static_cast<uint32_t>(unit.GetId());
        state.ownerId = entry.ownerId;
        state.unitType = static_cast<uint8_t>(unit.GetType());
        state.x = static_cast<uint32_t>(unit.x);
        state.y = static_cast<uint32_t>(unit.y);
        state.hp = static_cast<uint32_t>(unit.GetHp());
        state.maxHp = static_cast<uint32_t>(unit.GetMaxHp());

        // 현재는 이동만 구현된 단계.
        // 공격 상태 반영은 공격 처리 구현 시 연결한다.
        state.action = static_cast<uint8_t>(UnitAction::Move);

        message.units.push_back(state);
    }

    message.unitCount =
        static_cast<uint16_t>(message.units.size());

    // 마을회관 상태
    StructureStateInfo townHall;
    townHall.structureId = townHall_.id;
    townHall.hp = townHall_.hp;
    townHall.maxHp = townHall_.maxHp;
    message.structures.push_back(townHall);

    // 방어타워 상태
    for (const Tower& tower : towers_) {
        StructureStateInfo state;
        state.structureId = tower.id;
        state.hp = tower.hp;
        state.maxHp = tower.maxHp;

        message.structures.push_back(state);
    }

    message.structureCount =
        static_cast<uint16_t>(message.structures.size());

    return message;
}

BattleProgress BattleInstance::BuildBattleProgress() const
{
    BattleProgress progress{};
    progress.partyId = partyId_;

    progress.remainingSeconds = static_cast<uint16_t>((remainingMs_ + 999u) / 1000u);
    
    const size_t totalTowers = towers_.size();
    size_t destroyedTowers = 0;

    for (const Tower& tower : towers_) 
    {
        if (tower.destroyed) 
        {
            ++destroyedTowers;
        }
    }

    // 전체 파괴율에는 마을회관도 건물 한 개로 포함한다.
    const size_t totalBuildings = totalTowers + 1;
    size_t destroyedBuildings = destroyedTowers;

    if (townHall_.destroyed) 
    {
        ++destroyedBuildings;
    }

    progress.destructionPercent = static_cast<uint8_t>(destroyedBuildings * 100 / totalBuildings);

    if (townHall_.destroyed) {
        progress.stars = 3;
    }
    else if (totalTowers > 0 && destroyedTowers * 100 >= totalTowers * 60) {
        progress.stars = 2;
    }
    else if (totalTowers > 0 && destroyedTowers * 100 >= totalTowers * 30) {
        progress.stars = 1;
    }

    return progress;
}

void BattleInstance::CheckResult() 
{

}
