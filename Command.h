#pragma once
#include <cstdint>
#include <vector>

// 세션 스레드에서 워커 스레드로 넘기는 요청 하나
struct Command {
    enum class Type : uint8_t {
        Packet,         // 완성된 패킷 수신
        Disconnected    // 연결 종료
    };

    Type                 type = Type::Packet;
    int                  sessionId = 0;
    std::vector<uint8_t> packet;    // 헤더를 포함한 패킷 전체, Packet일 때만 사용
};