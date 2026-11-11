#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "NetworkClient.h"

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
	printf("보낼 문자열을 입력하세요. q 입력 시 종료\n");

	char line[256];
	while (client.IsConnected() && fgets(line, sizeof(line), stdin) != NULL) {
		if (line[0] == 'q' && (line[1] == '\n' || line[1] == '\0')) break;
		if (!client.QueuePacket(line, (int)strlen(line)))
			printf("전송 큐에 넣지 못했습니다.\n");
	}

	if (!client.IsConnected())
		err_display(client.LastError());

	client.Disconnect();
	NetworkClient::Cleanup();
	return 0;
}