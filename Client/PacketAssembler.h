#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// TCP 수신 바이트를 패킷 단위로 재조립 (TCP는 패킷 경계 없이 잘리거나 붙어서 도착)
// 받은 바이트를 이어 붙이고 헤더의 totalLength만큼 모이면 패킷 하나로 분리
class PacketAssembler {
public:
	// 패킷 최대 크기 (이보다 크거나 헤더보다 작으면 잘못된 패킷)
	static const uint32_t MAX_PACKET_SIZE = 1024 * 1024;

	// 수신 큐에서 꺼낸 바이트 이어 붙이기
	void Append(const char* data, size_t length);

	// 완성된 패킷 하나 꺼내기 (헤더 포함). 덜 모였거나 오류 상태면 false
	bool Pop(std::vector<uint8_t>& packet);

	// 잘못된 길이 수신 여부 (true면 이후 데이터를 믿을 수 없으므로 연결 종료 필요)
	bool HasError() const { return m_error; }

	void Clear();

	// 패킷 헤더의 messageType. 헤더보다 짧으면 0
	static uint16_t MessageTypeOf(const std::vector<uint8_t>& packet);

private:
	std::vector<uint8_t> m_buffer;
	bool m_error = false;
};