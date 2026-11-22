#pragma once
#include <windows.h>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "../LobbyState.h"
#include "../PacketCodec.h"
#include "ClientState.h"

class MapUI {
public:
    using SendFn = std::function<bool(const PacketCodec::Packet&)>;         // 서버로 패킷 전송
    using RequestIdFn = std::function<uint32_t()>;                          // 요청 번호 발급

    MapUI(const ClientState& state, SendFn send, RequestIdFn nextRequestId);

    
    void Draw(HDC hdc, const std::vector<LobbyMapInfo>& mapList);   // 클라이언트가 가진 맵 목록 표시
    bool OnMapClicked(uint8_t mapId);                               // 선택한 맵의 참가 요청 생성·전송

    
    bool HandleClick(int x, int y, const std::vector<LobbyMapInfo>& mapList);   // 마우스 좌표가 맵 버튼 위면 OnMapClicked 호출
    void OnJoined();                                                            // 참가 성공
    void OnRejected(uint32_t requestId, uint16_t errorCode);                    // 서버가 참가를 거절
    void Reset();                                                               // 로비 복귀 시 대기 상태 초기화

private:
    RECT ButtonRect(size_t index, size_t count) const;

    const ClientState& state_;
    SendFn             send_;
    RequestIdFn        nextRequestId_;
    uint32_t           pendingRequestId_ = 0;   // 응답을 기다리는 참가 요청 번호 (0이면 없음)
    std::wstring       message_;                // 화면 아래에 보여줄 안내 문구
};