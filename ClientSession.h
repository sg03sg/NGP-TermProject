#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <thread>
#include <atomic>
#include <vector>
#include "ThreadSafeQueue.h"
// #include "PacketBuffer.h"

class GameServer;

class ClientSession {
private:
    GameServer* server_;

    const int sessionId_;
    SOCKET socket_;
    // PacketBuffer recvBuffer_;
    ThreadSafeQueue<std::vector<char>> sendQueue_;
    std::thread recvThread_;
    std::thread sendThread_;
    std::atomic<bool> closing_;
    std::atomic<bool> closeNotified_;

    void RecvThread();
    void SendThread();

public:
    ClientSession(GameServer* server, SOCKET sock, int sessionId);
    ~ClientSession();

    void StartReceive();
    bool QueuePacket(const char* data, int length);
    void RequestClose();
    void Join();

    bool IsConnected() const { return !closing_; }
    int GetSessionId() const { return sessionId_; }
    SOCKET GetSocket() const { return socket_; }
};