#pragma once
#include <winsock2.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

class ClientSession;
class GameServer;

class SessionManager {
private:
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessions_;
    std::mutex mutex_;
    int nextId_;

public:
    SessionManager();
    ~SessionManager();

    int AddSession(SOCKET sock, GameServer* server);
    std::shared_ptr<ClientSession> FindSession(int sessionId);
    void SendToSessions(const std::vector<int>& sessionIds, const char* data, int length);
    void RemoveSession(int sessionId);

    void ReapClosed(); // 끊긴 세션을 맵에서 빼고 락 밖에서 Join (게임 스레드가 주기 호출)
    void Broadcast(const char* data, int length);
    void Clear();
};