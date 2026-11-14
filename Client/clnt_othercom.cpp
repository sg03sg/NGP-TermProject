#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "NetworkClient.h"

#include <conio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

int main(int argc, char* argv[])
{
	const char* serverip = SERVERIP;
	int port = SERVERPORT;
	if (argc >= 2) serverip = argv[1];
	if (argc >= 3) port = atoi(argv[2]);

	if (port <= 0 || port > 65535) {
		printf("잘못된 포트 번호: %s\n", argv[2]);
		return 1;
	}

	if (!NetworkClient::Startup()) return 1;

	NetworkClient client;
	if (!client.Connect(serverip, (unsigned short)port)) {
		WSASetLastError(client.LastError());
		err_quit("connect()");
	}
	printf("[TCP 클라이언트] 서버 접속: IP=%s, 포트=%d\n", serverip, port);
	printf("문자열을 입력하고 Enter를 누르면 전송합니다. q 입력 시 종료\n");

	char line[256];
	int lineLen = 0;
	bool running = true;

	while (running) {
		std::vector<char> data;
		while (client.PollReceived(data)) {
			printf("\n[수신] %d바이트: ", (int)data.size());
			fwrite(data.data(), 1, data.size(), stdout);
			printf("\n");
		}

		if (!client.IsConnected()) break;

		while (_kbhit()) {
			int c = _getch();
			if (c == 0 || c == 0xE0) {
				_getch();
				continue;
			}
			if (c == '\r') {
				printf("\n");
				line[lineLen] = '\0';
				if (strcmp(line, "q") == 0) {
					running = false;
					break;
				}
				if (lineLen > 0 && !client.QueuePacket(line, lineLen))
					printf("전송 큐에 넣지 못했습니다.\n");
				lineLen = 0;
			}
			else if (c == '\b') {
				if (lineLen > 0) {
					lineLen--;
					printf("\b \b");
				}
			}
			else if (lineLen < (int)sizeof(line) - 1) {
				line[lineLen++] = (char)c;
				putchar(c);
			}
		}

		Sleep(16);
	}

	if (!client.IsConnected()) {
		if (client.LastError() == 0)
			printf("서버가 연결을 종료했습니다.\n");
		else
			err_display(client.LastError());
	}

	client.Disconnect();
	NetworkClient::Cleanup();
	return 0;
}