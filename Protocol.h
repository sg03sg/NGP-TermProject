#pragma once

#include <cstdint>

constexpr std::uint32_t PACKET_HEADER_SIZE = 10;

struct PacketHeader {
	// 헤더, 본문 포함 전체 길이
	std::uint32_t totalLength = PACKET_HEADER_SIZE;
	// 메세지 종류 
	std::uint16_t messageType = 0;
	// 요청 ID
	std::uint32_t requestId = 0;
};

enum class MessageType : std::uint16_t {
	// 접속 맵 참가 
	WELCOME = 1,
	JOIN_MAP = 2,
	HELLO = 18,

	// 파티 관련 메세지 
	PARTY_STATE = 3,
	START_GAME = 4,
	LEAVE_PARTY = 16,
	LEAVE_APPROVED = 17,

	// 채팅 관련 메세지
	CHAT_SEND = 14,
	CHAT_MESSAGE = 15,


	// 전투 초기화, 소환 관련 메세지 
	BATTLE_INIT = 5,
	SPAWN_REQUEST = 6,
	UNIT_CREATED = 7,
	MANA_STATE = 8,
	REQUEST_REJECT = 9,

	// 전투 동기화, 종료 관련 메세지
	BATTLE_STATE = 10,
	BATTLE_PROGRESS = 11,
	BATTLE_RESULT = 12,
	RETURN_LOBBY = 13,

	// 중단 투표 관련 메세지 
	ABORT_VOTE_START = 19,
	ABORT_VOTE_CAST = 20,
	ABORT_VOTE_RESULT = 21,
	// 점 찍기 검증 확인용 
	MARKER_REQUEST = 22,
	MARKER_PLACED = 23
};