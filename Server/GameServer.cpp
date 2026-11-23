#include "GameServer.h"
#include "ClientSession.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace {
    // 워커 스레드가 한 틱에 처리하는 최대 요청 수
    constexpr int kMaxCommandsPerTick = 256;

    // 클라이언트가 LOBBY_STATE(24)를 처리하게 되면 true로 전환.
    constexpr bool kEnableLobbyBroadcast = false;
}

GameServer::GameServer() : listenSocket_(INVALID_SOCKET), running_(false), dispatcher_(*this) {

}

GameServer::~GameServer() {
    Stop();
}

bool GameServer::Start(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[GameServer] WSAStartup 실패" << std::endl;
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "[GameServer] 소켓 생성 실패: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    BOOL reuse = TRUE;   // 재시작 시 bind 실패 방지
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (::bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[GameServer] bind 실패: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[GameServer] listen 실패: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return false;
    }

    listenSocket_ = sock;
    running_ = true;
    std::cout << "[GameServer] 로비 방송: " << (kEnableLobbyBroadcast ? "켜짐" : "꺼짐") << std::endl;
    acceptThread_ = std::thread(&GameServer::AcceptLoop, this);
    workerThread_ = std::thread(&GameServer::WorkerLoop, this);

    std::cout << "[GameServer] 소켓 초기화 및 스레드 기동 완료 (Port: " << port << ")" << std::endl;
    return true;
}

void GameServer::AcceptLoop() {
    int consecutiveErrors = 0;
    while (running_) {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);

        SOCKET clientSock = accept(listenSocket_, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET) {
            if (!running_) break;
            std::cerr << "[GameServer] accept 에러: " << WSAGetLastError() << std::endl;
            // 에러가 반복돼도 CPU를 태우지 않도록 대기
            if (++consecutiveErrors >= 5) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        consecutiveErrors = 0;

        BOOL noDelay = TRUE;   // 작은 패킷 지연 방지 (Nagle off)
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

        std::cout << "[GameServer] 새 클라이언트 접속 수락. Socket: " << clientSock << std::endl;
        session_manager_.AddSession(clientSock, this);
    }
}

void GameServer::RemoveSession(int sessionId) {
    session_manager_.RemoveSession(sessionId);
}

void GameServer::BroadcastPacket(const char* data, int length) {
    session_manager_.Broadcast(data, length);
}

bool GameServer::PushRequest(Command command) {
    return requestQueue_.Push(std::move(command));
}

void GameServer::WorkerLoop() 
{
    using Clock = std::chrono::steady_clock;

    constexpr auto stateSendInterval = std::chrono::milliseconds(100);

    auto previous = Clock::now();
    auto nextStateSend = previous + stateSendInterval;

    while (running_) {
        // 1. 클라이언트 요청 처리
        Command cmd;

        for (int i = 0;
            i < kMaxCommandsPerTick && requestQueue_.TryPop(cmd);
            ++i) {
            dispatcher_.Dispatch(cmd);
        }

        // 2. 실제 경과 시간 계산
        const auto now = Clock::now();

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - previous);

        const uint32_t dtMs =
            static_cast<uint32_t>(elapsed.count());

        previous += elapsed;

        // 3. 전투별 이동 계산
        for (auto& entry : battles_) {
            entry.second->Update(dtMs);
        }

        // 4. 약 100ms마다 최신 전투 상태 전송
        const auto sendNow = Clock::now();

        if (sendNow >= nextStateSend) {
            for (const auto& entry : battles_) {
                const BattleState message =
                    entry.second->BuildBattleState();

                dispatcher_.SendBattleState(message);
            }

            // 밀린 횟수만큼 과거 상태를 연속 전송하지 않고,
            // 최신 상태 한 번을 보낸 뒤 다음 전송을 예약한다.
            nextStateSend = sendNow + stateSendInterval;
        }

        // 5. 끊긴 세션 정리
        session_manager_.ReapClosed();

        // 6. 기존 로비 방송
        const uint64_t nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                Clock::now().time_since_epoch()).count());

        if (kEnableLobbyBroadcast &&
            lobbyBroadcaster_.ShouldSend(nowMs)) {
            lobbyBroadcaster_.Tick(
                nowMs,
                partyManager_.BuildLobbyMaps(),
                session_manager_.GetSessionIds(),
                session_manager_);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GameServer::Stop() {
    // exchange로 중복 호출을 확실히 방지
    if (!running_.exchange(false)) return;

    // 리슨 소켓을 닫아 accept를 깨움
    SOCKET s = listenSocket_.exchange(INVALID_SOCKET);
    if (s != INVALID_SOCKET) closesocket(s);

    // accept 스레드를 먼저 join -> 이후 새 세션이 등록될 수 없음
    if (acceptThread_.joinable()) acceptThread_.join();
    if (workerThread_.joinable()) workerThread_.join();

    battles_.clear(); 

    // 안전하게 모든 세션 정리
    session_manager_.Clear();
    requestQueue_.Close();   // 세션이 모두 끝난 뒤 큐 닫기

    WSACleanup();
    std::cout << "[GameServer] 서버가 성공적으로 종료되었습니다." << std::endl;
}

BattleInstance* GameServer::FindBattle(PartyId partyId)
{
    auto it = battles_.find(partyId);
    return it == battles_.end() ? nullptr : it->second.get();
}

bool GameServer::AddBattle(
    PartyId partyId,
    std::unique_ptr<BattleInstance> battle)
{
    if (!battle) return false;

    return battles_.emplace(partyId, std::move(battle)).second;
}