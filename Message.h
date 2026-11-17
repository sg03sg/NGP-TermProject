#pragma once

#include <cstdint>
#include <string>
#include <vector>

using namespace std;

// 본문 포함 구조체 ======================================================================

struct PartyMemberInfo {
	uint32_t playerId = 0;
	uint16_t nmaeLength = 0;
	string name;
};

struct PlayerInitInfo {
	uint32_t playerId = 0;
	uint16_t mana = 0;
	uint16_t maxMana = 0;
};

struct StructureInitInfo {
	uint32_t structureId = 0;
	uint8_t type = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t hp = 0;
	uint32_t maxHp = 0;
};

struct UnitStateInfo {
	uint32_t unitId = 0;
	uint32_t ownerId = 0;
	uint8_t unitType = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t hp = 0;
	uint32_t maxHp = 0;
	uint8_t action = 0;
};

struct StructureStateInfo {
	uint32_t structureId = 0;
	uint32_t hp = 0;
	uint32_t maxHp = 0;
};

// 접속 및 맵 참가 관련 구조체	========================================================

struct Welcome {
	uint32_t playerId = 0;
};

struct JoinMap {
	uint8_t mapId = 0;
};

struct Hello {
	uint16_t nmaeLength = 0;
	string name;
};

// 파티 관련 구조체 ======================================================================

struct PartyState {
	uint32_t partyId = 0;
	uint8_t mapId = 0;
	uint32_t leaderId = 0;
	uint8_t memberCount = 0;
	vector<PartyMemberInfo> members;
};

struct StartGame {
	uint32_t partyId = 0;
};

struct LeaveParty {
	uint32_t partyId = 0;
};

struct LeaveApproved {
	uint32_t partyId = 0;
};

// 채팅 관련 구조체 ======================================================================

struct ChatSend {
	uint16_t textLength = 0;
	string text;
};

struct ChatMessage {
	uint32_t senderId = 0;
	uint16_t textLength = 0;
	string text;
};

// 전투 초기화 및 소환 관련 구조체 ===========================================================

struct BattleInit {
	uint32_t partyId = 0;
	uint8_t mapId = 0;
	uint16_t remainingSeconds = 0;
	uint8_t playerCount = 0;
	vector<PlayerInitInfo> players;
	uint16_t structureCount = 0;
	vector<StructureInitInfo>structures;
};


struct SpawnRequest {
	uint8_t unitType = 0;
	uint32_t x = 0;
	uint32_t y = 0;
};

struct UnitCreated {
	uint32_t unitId = 0;
	uint32_t ownerId = 0;
	uint8_t unitType = 0;
	uint32_t x = 0;
	uint32_t y = 0;
	uint32_t hp = 0;
	uint32_t maxHp = 0;
};

struct ManaState {
	uint16_t mana = 0;
	uint16_t maxMana = 0;
};

struct RequestRejected {
	uint16_t rejectedType = 0;
	uint16_t errorCode = 0;
};

// 전투 동기화 및 종료 관련 구조체 ===============================================================

struct BattleState {
	uint32_t partyId = 0;
	uint32_t serverTick = 0;
	uint16_t unitCount = 0;
	vector<UnitStateInfo> units;
	uint16_t structureCount = 0;
	vector<StructureStateInfo> structures;
};

struct BattleProgress {
	uint32_t partyId = 0;
	uint16_t remainingSeconds = 0;
	uint8_t destructionPercent = 0;
	uint8_t stars = 0;
};

struct BattleResult {
	uint32_t partyId = 0;
	uint8_t reason = 0;
	uint8_t destructionPercent = 0;
	uint8_t stars = 0;
	uint16_t returnSeconds = 0;
};

struct ReturnLobby {
	uint32_t partyId = 0;
};

// 중단 투표 관련 구조체 =================================================================

struct AbortVoteStart {
	uint32_t partyId = 0;
	uint32_t leaverId = 0;
	uint16_t voteSeconds = 0;
	uint8_t requestedVotes = 0;
	uint8_t totalVoters = 0;
};

struct AbortVoteCast {
	uint32_t partyId = 0;
	uint8_t stop = 0;
};

struct AbortVoteResult {
	uint32_t partyId = 0;
	uint8_t aborted = 0;
	uint8_t stopVotes = 0;
	uint8_t totalVoters = 0;
};

// 좌표 테스트용 구조체 ===================================================================

struct MarkerRequest {
	uint32_t x = 0;
	uint32_t y = 0;
};

struct MarkerPlaced {
	uint32_t senderId = 0;
	uint32_t x = 0;
	uint32_t y = 0;
};