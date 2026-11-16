#include "PacketBuffer.h"

void PacketBuffer::Append(const uint8_t* data, size_t size)
{
    if (size == 0) {
        return;
    }

    if (readPosition > 0) {
        buffer.erase(buffer.begin(), buffer.begin() + readPosition);
        readPosition = 0;
    }

    buffer.insert(buffer.end(), data, data + size);
}

ExtractResult PacketBuffer::TryExtractPacket(Packet& out)
{
    const size_t available = buffer.size() - readPosition;

    if (available < 4) {
        return ExtractResult::More;
    }

    uint32_t totalLength = 0;

    for (size_t i = 0; i < 4; ++i) {
        totalLength = (totalLength << 8) | buffer[readPosition + i];
    }

    const uint32_t maxPacketSize = 16u * 1024u * 1024u;

    if (totalLength < PACKET_HEADER_SIZE || totalLength > maxPacketSize) {
        return ExtractResult::InvalidLength;
    }
 
    if (available < totalLength) {
        return ExtractResult::More;
    }

    auto first = buffer.begin() + readPosition;
    out.assign(first, first + totalLength);

    readPosition += totalLength;

    if (readPosition == buffer.size()) {
        Clear();
    }

    return ExtractResult::Ready;
}

void PacketBuffer::Clear()
{
    buffer.clear();
    readPosition = 0;
}