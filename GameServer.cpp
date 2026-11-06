#include "GameServer.h"
#include "ClientSession.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

GameServer::GameServer()
    : listenSocket_(INVALID_SOCKET), running_(false) {
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

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        std::cerr << "[GameServer] 소켓 생성 실패: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket_, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[GameServer] bind 실패: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket_);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[GameServer] listen 실패: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket_);
        WSACleanup();
        return false;
    }

    running_ = true;
    acceptThread_ = std::thread(&GameServer::AcceptLoop, this);
    workerThread_ = std::thread(&GameServer::WorkerLoop, this);

    std::cout << "[GameServer] 소켓 초기화 및 스레드 기동 완료 (Port: " << port << ")" << std::endl;
    return true;
}

void GameServer::AcceptLoop() {
    while (running_) {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);

        SOCKET clientSock = accept(listenSocket_, (SOCKADDR*)&clientAddr, &addrLen);
        if (clientSock == INVALID_SOCKET) {
            if (!running_) break;
            std::cerr << "[GameServer] accept 에러: " << WSAGetLastError() << std::endl;
            continue;
        }

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
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GameServer::Stop() {
    if (!running_) return;
    running_ = false;

    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_);
        listenSocket_ = INVALID_SOCKET;
    }

    session_manager_.Clear();

    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }

    if (workerThread_.joinable()) {
        workerThread_.join();
    }

    WSACleanup();
    std::cout << "[GameServer] 서버가 성공적으로 종료되었습니다." << std::endl;
}