#include "GameServer.h"
#include "ClientSession.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

GameServer::GameServer() : listenSocket_(INVALID_SOCKET), running_(false) {

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

    if (bind(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
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
            // [수정] 에러가 반복돼도 CPU를 태우지 않도록 대기
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

void GameServer::WorkerLoop() {
    while (running_) {
        // 추후 requestQueue_ 처리 로직이 들어갈 자리
        session_manager_.ReapClosed();   // 끊긴 세션 정리
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

    // 안전하게 모든 세션 정리
    session_manager_.Clear();

    WSACleanup();
    std::cout << "[GameServer] 서버가 성공적으로 종료되었습니다." << std::endl;
}