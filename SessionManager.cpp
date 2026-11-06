#include "SessionManager.h"
#include "ClientSession.h"
#include <iostream>

SessionManager::SessionManager() : nextId_(1) {}

SessionManager::~SessionManager() {
    Clear();
}

int SessionManager::AddSession(SOCKET sock, GameServer* server) {
    std::lock_guard<std::mutex> lock(mutex_);
    int sessionId = nextId_++;
    auto session = std::make_shared<ClientSession>(server, sock, sessionId);
    sessions_[sessionId] = session;

    session->StartReceive();

    std::cout << "[SessionManager] 세션 등록 완료 (Session ID: " << sessionId << ")" << std::endl;
    return sessionId;
}

std::shared_ptr<ClientSession> SessionManager::FindSession(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        return it->second;
    }
    return nullptr;
}

void SessionManager::SendToSessions(const std::vector<int>& sessionIds, const char* data, int length) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (int id : sessionIds) {
        auto it = sessions_.find(id);
        if (it != sessions_.end() && it->second && it->second->IsConnected()) {
            it->second->QueuePacket(data, length);
        }
    }
}

void SessionManager::RemoveSession(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) {
        std::cout << "[SessionManager] 세션 제거 완료 (Session ID: " << sessionId << ")" << std::endl;
        sessions_.erase(it);
    }
}

void SessionManager::Broadcast(const char* data, int length) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : sessions_) {
        if (pair.second && pair.second->IsConnected()) {
            pair.second->QueuePacket(data, length);
        }
    }
}

void SessionManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : sessions_) {
        if (pair.second) {
            pair.second->RequestClose();
            pair.second->Join();
        }
    }
    sessions_.clear();
}