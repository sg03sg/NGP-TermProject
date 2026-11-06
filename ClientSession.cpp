#include "ClientSession.h"
#include "GameServer.h"
#include <iostream>

ClientSession::ClientSession(GameServer* server, SOCKET sock, int sessionId)
    : server_(server), socket_(sock), sessionId_(sessionId), closing_(false), closeNotified_(false) {
}

ClientSession::~ClientSession() {
    RequestClose();
    Join();
}

void ClientSession::StartReceive() {
    closing_ = false;
    closeNotified_ = false;
    recvThread_ = std::thread(&ClientSession::RecvThread, this);
    sendThread_ = std::thread(&ClientSession::SendThread, this);
}

void ClientSession::RecvThread() {
    char buffer[512];
    while (!closing_) {
        int recvBytes = recv(socket_, buffer, sizeof(buffer), 0);
        if (recvBytes <= 0) {
            std::cout << "[ClientSession] 세션 (" << sessionId_ << ") 연결 종료 감지." << std::endl;
            RequestClose();
            break;
        }

        // 수신 데이터 처리 (PacketBuffer 및 CommandDispatcher 연동)
    }
}

void ClientSession::SendThread() {
    std::vector<char> packetData;
    while (!closing_) {
        if (sendQueue_.WaitPop(packetData)) {
            if (closing_) break;
            int sent = send(socket_, packetData.data(), static_cast<int>(packetData.size()), 0);
            if (sent == SOCKET_ERROR) {
                RequestClose();
                break;
            }
        }
        else {
            break;
        }
    }
}

bool ClientSession::QueuePacket(const char* data, int length) {
    if (closing_) return false;
    std::vector<char> packet(data, data + length);
    sendQueue_.Push(packet);
    return true;
}

void ClientSession::RequestClose() {
    bool expected = false;
    if (closing_.compare_exchange_strong(expected, true)) {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
        sendQueue_.Close();
    }
}

void ClientSession::Join() {
    if (recvThread_.joinable()) {
        recvThread_.join();
    }
    if (sendThread_.joinable()) {
        sendThread_.join();
    }

    if (!closeNotified_) {
        closeNotified_ = true;
        if (server_) {
            server_->RemoveSession(sessionId_);
        }
    }
}