#pragma once

#include <cstdint>
#include <vector>

#include "Protocol.h"

using namespace std;

enum class ExtractResult {
    More,       // 아직 데이터가 부족함
    Ready,          // 패킷 하나를 추출했음
    InvalidLength   // 전체 길이가 잘못됨
};

class PacketBuffer {
public:
    using Packet = vector<uint8_t>;

    // 데이터를 누적
    void Append(const uint8_t* data, size_t size);

    // 완성된 패킷 하나를 out에 넣는다
    ExtractResult TryExtractPacket(Packet& out);

    // 누적한 데이터를 초기화
    void Clear();

private:
    Packet buffer;
    size_t readPosition = 0;
};