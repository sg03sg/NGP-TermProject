#pragma once
#include <cstddef>
#include "BattleInstance.h"

// 스테이지별 전투 맵 데이터 생성
// 인원 수별 맵을 구성하기 전까지 인원과 무관하게 같은 배치를 사용
// 없는 스테이지면 false
bool BuildMapData(MapDataId mapId, size_t playerCount, MapData& out);
