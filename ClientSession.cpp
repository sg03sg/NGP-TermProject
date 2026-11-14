#include "ClientSession.h"
#include "GameServer.h"
#include <iostream>
#include <algorithm>
#include <exception>
#include <string>   // std::to_string
#include <new>      // std::bad_alloc

// 초기화 리스트 순서를 선언 순서(server_, sessionId_, socket_)에 맞춤
ClientSession::ClientSession(GameServer* server, SOCKET sock, int sessionId) : server_(server), sessionId_(sessionId), socket_(sock), closing_(false), closeNotified_(false) {

}

ClientSession::~ClientSession() {
    RequestClose();
    Join();
}

bool ClientSession::StartReceive() {
    // 소켓 옵션은 실패해도 치명적이지 않으므로 결과를 무시한다.
    BOOL nodelay = TRUE;   // 작은 패킷을 지연 없이 전송
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));
    BOOL keepAlive = TRUE; // 비정상 단절(전원/랜선) 감지에 도움
    setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&keepAlive), sizeof(keepAlive));

    try {
        recvThread_ = std::thread(&ClientSession::RecvThread, this);
        sendThread_ = std::thread(&ClientSession::SendThread, this);
    }
    catch (const std::exception& e) {
        // 스레드가 하나만 만들어진 채로 소멸하면 std::terminate가 호출되므로 반드시 정리한다.
        std::cout << "[ClientSession] 세션 (" << sessionId_ << ") 스레드 생성 실패: " << e.what() << std::endl;
        RequestClose();
        Join();
        return false;
    }
    return true;
}

void ClientSession::RecvThread() {
    char buffer[512];
    while (!closing_) {
        int recvBytes = recv(socket_, buffer, sizeof(buffer), 0);

        if (recvBytes == SOCKET_ERROR && WSAGetLastError() == WSAEINTR) {
            continue;   // 인터럽트는 재시도
        }
        if (recvBytes <= 0) {
            // 서버가 먼저 끊은 경우(closing_)에는 로그를 남기지 않는다.
            if (!closing_) {
                std::cout << "[ClientSession] 세션 (" << sessionId_ << ") 연결 종료 감지." << (recvBytes == SOCKET_ERROR ? " (오류 코드: " + std::to_string(WSAGetLastError()) + ")" : "") << std::endl;
            }
            break;
        }

        // 수신 데이터 처리 (PacketBuffer 및 CommandDispatcher 연동)
    }

    RequestClose();   // 송신 스레드도 깨움
    NotifyClosed();   // 정리는 소유자가 한다 (여기서 RemoveSession 직접 호출 금지)
}

void ClientSession::SendThread() {
    std::vector<char> packetData;
    while (sendQueue_.WaitPop(packetData)) {          // 큐가 닫히면 false → 종료
        size_t offset = 0;
        while (offset < packetData.size()) {
            if (closing_) return;

            const int remain = static_cast<int>(std::min<size_t>(packetData.size() - offset, 64 * 1024));
            const int sent = send(socket_, packetData.data() + offset, remain, 0);

            if (sent == SOCKET_ERROR) {
                if (WSAGetLastError() == WSAEINTR) continue;   // 인터럽트는 재시도
                RequestClose();
                return;
            }
            if (sent == 0) {                                   // 진행 없음 → 무한 루프 방지
                RequestClose();
                return;
            }
            offset += static_cast<size_t>(sent);
        }
    }
}

bool ClientSession::QueuePacket(const char* data, int length) {
    if (closing_ || data == nullptr || length <= 0) return false;

    // 수신이 느린 클라이언트가 큐를 무한정 키우지 못하게 한다.
    if (sendQueue_.Size() >= kMaxPendingPackets) {
        std::cout << "[ClientSession] 세션 (" << sessionId_ << ") 송신 큐 초과, 연결 종료." << std::endl;
        RequestClose();
        return false;
    }

    try {
        return sendQueue_.Push(std::vector<char>(data, data + length));
    }
    catch (const std::bad_alloc&) {
        // 메모리 부족이 게임 스레드로 번지지 않게 이 세션만 끊는다.
        RequestClose();
        return false;
    }
}

void ClientSession::RequestClose() {
    bool expected = false;
    if (closing_.compare_exchange_strong(expected, true)) {
        shutdown(socket_, SD_BOTH);
        sendQueue_.Close();
    }
}

void ClientSession::NotifyClosed() {
    if (!closeNotified_.exchange(true)) {
        // TODO: requestQueue_ 가 생기면 여기서 "연결 종료 이벤트(sessionId_)"를 Push
        // 현재는 SessionManager::ReapClosed() 가 IsConnected()==false 로 감지해 정리한다.
    }
}

void ClientSession::Join() {
    if (recvThread_.joinable()) {
        recvThread_.join();
    }
    if (sendThread_.joinable()) {
        sendThread_.join();
    }

    // 두 스레드가 끝난 뒤에만 소켓을 닫는다.
    // 여기서 server_->RemoveSession() 을 호출하지 않는다 (재진입 데드락의 원인).
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}