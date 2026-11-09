#include "SessionManager.h"
#include "ClientSession.h"
#include <iostream>

SessionManager::SessionManager() : nextId_(1) {}

SessionManager::~SessionManager() {
    Clear();
}

int SessionManager::AddSession(SOCKET sock, GameServer* server) {
    std::shared_ptr<ClientSession> session;
    int sessionId;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessionId = nextId_++;
        session = std::make_shared<ClientSession>(server, sock, sessionId);
        sessions_[sessionId] = session;
    }
    session->StartReceive();   // 락 밖에서 스레드 기동

    std::cout << "[SessionManager] 세션 등록 완료 (Session ID: " << sessionId << ")" << std::endl;
    return sessionId;
}

std::shared_ptr<ClientSession> SessionManager::FindSession(int sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    return (it != sessions_.end()) ? it->second : nullptr;
}

void SessionManager::SendToSessions(const std::vector<int>& sessionIds, const char* data, int length) {
    // 락 안에서는 shared_ptr 수집만, 호출은 락 밖에서
    std::vector<std::shared_ptr<ClientSession>> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (int id : sessionIds) {
            auto it = sessions_.find(id);
            if (it != sessions_.end() && it->second) targets.push_back(it->second);
        }
    }
    for (auto& s : targets) {
        if (s->IsConnected()) s->QueuePacket(data, length);
    }
}

void SessionManager::RemoveSession(int sessionId) {
    // 락 안에서는 맵에서 빼기만 하고, 소멸(=Join)은 락 밖에서 -> 재진입 데드락 제거
    std::shared_ptr<ClientSession> removed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) return;
        removed = std::move(it->second);
        sessions_.erase(it);
    }
    removed->RequestClose();
    removed->Join();
    std::cout << "[SessionManager] 세션 제거 완료 (Session ID: " << sessionId << ")" << std::endl;
}

void SessionManager::ReapClosed() {
    std::vector<std::shared_ptr<ClientSession>> closed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (it->second && !it->second->IsConnected()) {
                closed.push_back(std::move(it->second));
                it = sessions_.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    for (auto& s : closed) {
        int id = s->GetSessionId();
        s->Join();   // 호출 스레드는 게임 스레드이므로 자기 join 문제 없음
        std::cout << "[SessionManager] 세션 제거 완료 (Session ID: " << id << ")" << std::endl;
    }
}

void SessionManager::Broadcast(const char* data, int length) {
    std::vector<std::shared_ptr<ClientSession>> targets;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : sessions_) {
            if (pair.second) targets.push_back(pair.second);
        }
    }
    for (auto& s : targets) {
        if (s->IsConnected()) s->QueuePacket(data, length);
    }
}

void SessionManager::Clear() {
    // 맵을 통째로 옮겨 놓고 락 밖에서 종료 -> 순회 중 erase / 락 재획득 문제 제거
    std::unordered_map<int, std::shared_ptr<ClientSession>> local;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        local.swap(sessions_);
    }
    for (auto& pair : local) pair.second->RequestClose();   // 먼저 전원 깨우고
    for (auto& pair : local) pair.second->Join();           // 그다음 join
}