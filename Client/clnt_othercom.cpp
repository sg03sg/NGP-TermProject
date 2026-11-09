#define _CRT_SECURE_NO_WARNINGS
#include "common.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

DWORD WINAPI ClientThread(LPVOID arg)
{
	SOCKET sock = (SOCKET)arg;

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	const char* serverip = SERVERIP;
	int port = SERVERPORT;
	if (argc >= 2) serverip = argv[1];
	if (argc >= 3) port = atoi(argv[2]);

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	if (inet_pton(AF_INET, serverip, &serv_addr.sin_addr) != 1) {
		printf("잘못된 IP 주소: %s\n", serverip);
		return 1;
	}
	if (port <= 0 || port > 65535) {
		printf("잘못된 포트 번호: %s\n", argv[2]);
		return 1;
	}
	serv_addr.sin_port = htons((u_short)port);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	retval = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	printf("[TCP 클라이언트] 서버 접속: IP=%s, 포트=%d\n", serverip, port);

	HANDLE hThread = CreateThread(NULL, 0, ClientThread, (LPVOID)sock, 0, NULL);
	if (hThread == NULL) {
		printf("스레드 생성 실패\n");
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);

	closesocket(sock);
	WSACleanup();
	return 0;
}