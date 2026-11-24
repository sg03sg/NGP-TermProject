#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include <chrono>
#include <map>
#include "BattleInstance.h"
#include "BattleManager.h"
#include "SessionManager.h"
#include "ThreadSafeQueue.h"
#include "LobbyBroadcaster.h"
#include "PartyManager.h"
#include "Command.h"
#include "CommandDispatcher.h"

class GameServer {
private:
    std::atomic<SOCKET> listenSocket_;   // Stop()과 AcceptLoop의 동시 접근 방지
    std::atomic<bool> running_;
    ThreadSafeQueue<Command> requestQueue_;
    std::thread acceptThread_;
    std::thread workerThread_;
    SessionManager session_manager_;
    PartyManager partyManager_;
    BattleManager battleManager_;
    LobbyBroadcaster lobbyBroadcaster_;
    CommandDispatcher dispatcher_;

    void AcceptLoop();
    void WorkerLoop();

public:
    GameServer();
    ~GameServer();

    bool Start(int port);
    void Stop();

    SessionManager& GetSessionManager() { return session_manager_; }
    PartyManager& GetPartyManager() { return partyManager_; }
    LobbyBroadcaster& GetLobbyBroadcaster() { return lobbyBroadcaster_; }
    BattleManager& GetBattleManager() { return battleManager_; }
    void RemoveSession(int sessionId);
    void BroadcastPacket(const char* data, int length);
    bool PushRequest(Command command);   // 요청 큐에 요청 추가, 세션 스레드에서 호출
    BattleInstance* FindBattle(PartyId partyId);
};