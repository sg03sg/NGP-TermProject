#include "SessionManager.h"
#include "ClientSession.h"
#include <iostream>
#include <exception>

SessionManager::SessionManager() : nextId_(1) {}

SessionManager::~SessionManager() {
    Clear();
}

// 실패하면 -1을 반환한다. 이 경우 sock은 이 함수 안에서 이미 닫혔으므로 호출자는 다시 닫지 않는다.
int SessionManager::AddSession(SOCKET sock, GameServer* server) {
    std::shared_ptr<ClientSession> session;
    int sessionId = -1;
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        sessionId = nextId_++;
        session = std::make_shared<ClientSession>(server, sock, sessionId);
        sessions_[sessionId] = session;
    }
    catch (const std::exception& e) {
        std::cout << "[SessionManager] 세션 생성 실패: " << e.what() << std::endl;
        if (sessionId != -1) {
            std::lock_guard<std::mutex> lock(mutex_);
            sessions_.erase(sessionId);
        }
        if (!session) closesocket(sock);   // 세션이 만들어졌다면 소멸자가 소켓을 닫는다
        return -1;
    }

    // 락 밖에서 스레드 기동
    if (!session->StartReceive()) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            sessions_.erase(sessionId);
        }
        std::cout << "[SessionManager] 세션 등록 실패 (Session ID: " << sessionId << ")" << std::endl;
        return -1;
    }

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
    if (!removed) return;
    removed->RequestClose();
    removed->Join();
    std::cout << "[SessionManager] 세션 제거 완료 (Session ID: " << sessionId << ")" << std::endl;
}

void SessionManager::ReapClosed() {
    std::vector<std::shared_ptr<ClientSession>> closed;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            if (!it->second || !it->second->IsConnected()) {
                if (it->second) closed.push_back(std::move(it->second));
                it = sessions_.erase(it);   // nullptr 항목도 함께 정리
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
    for (auto& pair : local) if (pair.second) pair.second->RequestClose();   // 먼저 전원 깨우고
    for (auto& pair : local) if (pair.second) pair.second->Join();           // 그다음 join
}