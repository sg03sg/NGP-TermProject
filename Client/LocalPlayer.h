#pragma once

#include <cstdint>
#include <string>

// 내 플레이어 정보
// 메인 스레드 전용 (네트워크 스레드는 접근하지 않으므로 락 불필요)
struct LocalPlayer {
	enum class Phase {
		WaitingWelcome,     // HELLO 전송 후 WELCOME 대기
		Joined              // WELCOME 수신, playerId 확정
	};

	Phase       phase = Phase::WaitingWelcome;
	std::string nickname;           // 입력한 닉네임 (콘솔 코드 페이지, 화면 출력용)
	uint32_t    playerId = 0;       // WELCOME으로 받은 내 ID (Joined일 때만 유효)
	uint32_t    nextRequestId = 1;  // 다음 요청 번호 (0은 서버 방송용이라 1부터 사용)

	uint32_t IssueRequestId() { return nextRequestId++; }

	bool IsMe(uint32_t id) const { return phase == Phase::Joined && id == playerId; }
};