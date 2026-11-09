#include "ClientSession.h"
#include "GameServer.h"
#include <iostream>

ClientSession::ClientSession(GameServer* server, SOCKET sock, int sessionId) : server_(server), socket_(sock), sessionId_(sessionId), closing_(false), closeNotified_(false) {

}

ClientSession::~ClientSession() {
    RequestClose();
    Join();
}

void ClientSession::StartReceive() {
    recvThread_ = std::thread(&ClientSession::RecvThread, this);
    sendThread_ = std::thread(&ClientSession::SendThread, this);
}

void ClientSession::RecvThread() {
    char buffer[512];
    while (!closing_) {
        int recvBytes = recv(socket_, buffer, sizeof(buffer), 0);
        if (recvBytes <= 0) {
            std::cout << "[ClientSession] 세션 (" << sessionId_ << ") 연결 종료 감지." << std::endl;
            break;
        }

        // 수신 데이터 처리 (PacketBuffer 및 CommandDispatcher 연동)
    }

    RequestClose();   // 송신 스레드도 깨움
    NotifyClosed();   // 정리는 소유자가 한다 (여기서 RemoveSession 직접 호출 금지)
}

void ClientSession::SendThread() {
    std::vector<char> packetData;
    while (!closing_) {
        if (sendQueue_.WaitPop(packetData)) {
            if (closing_) break;
            size_t offset = 0;
            while (offset < packetData.size()) {
                int sent = send(socket_, packetData.data() + offset, static_cast<int>(packetData.size() - offset), 0);
                if (sent == SOCKET_ERROR) {
                    RequestClose();
                    return;
                }
                offset += static_cast<size_t>(sent);
            }
        }
    }
}

bool ClientSession::QueuePacket(const char* data, int length) {
    if (closing_ || data == nullptr || length <= 0) {
        return false;
    }
    return sendQueue_.Push(std::vector<char>(data, data + length));
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