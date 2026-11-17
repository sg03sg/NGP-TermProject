#include "PacketAssembler.h"
#include "../Protocol.h"

void PacketAssembler::Append(const char* data, size_t length) {
	if (data == nullptr || length == 0) return;
	m_buffer.insert(m_buffer.end(), (const uint8_t*)data, (const uint8_t*)data + length);
}

bool PacketAssembler::Pop(std::vector<uint8_t>& packet) {
	if (m_error) return false;
	if (m_buffer.size() < 4) return false;                  // totalLength(4바이트) 수신 대기

	uint32_t totalLength = ((uint32_t)m_buffer[0] << 24) | ((uint32_t)m_buffer[1] << 16)
		| ((uint32_t)m_buffer[2] << 8) | (uint32_t)m_buffer[3];

	if (totalLength < PACKET_HEADER_SIZE || totalLength > MAX_PACKET_SIZE) {
		m_error = true;
		return false;
	}
	if (m_buffer.size() < totalLength) return false;      // 본문 수신 대기

	packet.assign(m_buffer.begin(), m_buffer.begin() + totalLength);
	m_buffer.erase(m_buffer.begin(), m_buffer.begin() + totalLength);
	return true;
}

void PacketAssembler::Clear() {
	m_buffer.clear();
	m_error = false;
}

uint16_t PacketAssembler::MessageTypeOf(const std::vector<uint8_t>& packet) {
	if (packet.size() < PACKET_HEADER_SIZE) return 0;
	return (uint16_t)((packet[4] << 8) | packet[5]);
}