#include <iostream>
#include "GameServer.h"

#define SERVER_PORT 9000

int main() {
    GameServer server;

    if (!server.Start(SERVER_PORT)) {
        std::cerr << "서버 실행 실패" << std::endl;
        return 1;
    }

    std::cout << "서버 실행 성공. Enter를 눌러 종료." << std::endl;
    std::cin.get();

    
    server.Stop();
    return 0;
}