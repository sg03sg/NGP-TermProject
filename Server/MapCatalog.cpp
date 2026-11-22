#include "MapCatalog.h"

namespace {
    struct TowerSpec {
        StructureKind kind;
        int32_t       x;
        int32_t       y;
    };

    constexpr uint32_t kTowerHp = 300;
    constexpr uint32_t kTownHallHp = 600;
    constexpr uint32_t kTownHallId = 1;
    constexpr int32_t  kTownHallX = 642;
    constexpr int32_t  kTownHallY = 392;

    // 솔로 게임의 스테이지별 타워 배치
    const TowerSpec kStage1[] = {
        { StructureKind::Cannon, 625, 380 },
        { StructureKind::Cannon, 670, 390 },
        { StructureKind::Cannon, 655, 420 },
        { StructureKind::Cannon, 610, 410 },
    };

    const TowerSpec kStage2[] = {
        { StructureKind::Cannon, 686, 412 },
        { StructureKind::Cannon, 640, 445 },
        { StructureKind::Cannon, 583, 398 },
        { StructureKind::Cannon, 645, 366 },
        { StructureKind::Mortar, 658, 423 },
        { StructureKind::Mortar, 613, 378 },
    };

    const TowerSpec kStage3[] = {
        { StructureKind::Cannon,  686, 412 },
        { StructureKind::Cannon,  640, 445 },
        { StructureKind::Cannon,  583, 398 },
        { StructureKind::Cannon,  645, 366 },
        { StructureKind::Mortar,  658, 423 },
        { StructureKind::Mortar,  613, 378 },
        { StructureKind::Inferno, 659, 378 },
        { StructureKind::Inferno, 610, 410 },
    };

    // 마을회관 다음 번호부터 타워 ID 발급
    template <size_t N>
    void AddTowers(MapData& out, const TowerSpec(&specs)[N])
    {
        for (const TowerSpec& spec : specs) {
            Tower tower;
            tower.id = kTownHallId + 1 + static_cast<uint32_t>(out.towers.size());
            tower.pos.x = spec.x;
            tower.pos.y = spec.y;
            tower.hp = kTowerHp;
            tower.maxHp = kTowerHp;
            tower.destroyed = false;
            tower.type = static_cast<uint8_t>(spec.kind);
            out.towers.push_back(tower);
        }
    }
}

bool BuildMapData(MapDataId mapId, size_t playerCount, MapData& out)
{
    // 인원 수별 배치가 생기면 playerCount로 맵을 고름
    (void)playerCount;

    out = MapData{};
    out.id = mapId;

    switch (mapId) {
    case 1: AddTowers(out, kStage1); break;
    case 2: AddTowers(out, kStage2); break;
    case 3: AddTowers(out, kStage3); break;
    default: return false;
    }

    out.townHall.id = kTownHallId;
    out.townHall.pos.x = kTownHallX;
    out.townHall.pos.y = kTownHallY;
    out.townHall.hp = kTownHallHp;
    out.townHall.maxHp = kTownHallHp;
    out.townHall.destroyed = false;
    return true;
}
