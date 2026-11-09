#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "SessionManager.h"
#include "ThreadSafeQueue.h"

class GameServer {
private:
    std::atomic<SOCKET> listenSocket_;   // Stop()과 AcceptLoop의 동시 접근 방지
    std::atomic<bool> running_;
    // ThreadSafeQueue<Command> requestQueue_;
    std::thread acceptThread_;
    std::thread workerThread_;

    SessionManager session_manager_;

    void AcceptLoop();
    void WorkerLoop();

public:
    GameServer();
    ~GameServer();

    bool Start(int port);
    void Stop();

    SessionManager& GetSessionManager() { return session_manager_; }
    void RemoveSession(int sessionId);
    void BroadcastPacket(const char* data, int length);
};