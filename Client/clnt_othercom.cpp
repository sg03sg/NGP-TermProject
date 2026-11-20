#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "NetworkClient.h"
#include "EncodingUtil.h"
#include "LocalPlayer.h"
#include "../PacketCodec.h"
#include "../PacketBuffer.h"
#include "../GameFramework.h"
#include "GameWindow.h"

#include <conio.h>
#include <string>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

static bool SendPacket(NetworkClient& client, const PacketCodec::Packet& packet)
{
	if (packet.empty()) return false;
	return client.QueuePacket(reinterpret_cast<const char*>(packet.data()), (int)packet.size());
}

static bool SendMarker(NetworkClient& client, LocalPlayer& player, uint32_t x, uint32_t y)
{
	MarkerRequest marker;
	marker.x = x;
	marker.y = y;

	uint32_t requestId = player.IssueRequestId();
	if (!SendPacket(client, PacketCodec::Encode(marker, requestId))) return false;

	printf("[송신] MARKER_REQUEST (%u, %u) 요청번호 %u\n", x, y, requestId);
	return true;
}

// 패킷 헤더의 messageType
static uint16_t MessageTypeOf(const PacketCodec::Packet& packet)
{
	if (packet.size() < PACKET_HEADER_SIZE) return 0;
	return (uint16_t)((packet[4] << 8) | packet[5]);
}

static void HandlePacket(const PacketCodec::Packet& packet, LocalPlayer& player)
{
	uint16_t type = MessageTypeOf(packet);

	switch (static_cast<MessageType>(type)) {
	case MessageType::WELCOME: {
		Welcome welcome;
		if (!PacketCodec::Decode(packet, welcome)) {
			printf("[오류] WELCOME 해석 실패\n");
			return;
		}
		if (player.phase == LocalPlayer::Phase::Joined) {
			printf("[무시] 이미 WELCOME을 받았습니다.\n");
			return;
		}

		player.playerId = welcome.playerId;
		player.phase = LocalPlayer::Phase::Joined;
		printf("[수신] WELCOME: 내 플레이어 ID = %u\n", player.playerId);
		printf("좌표를 \"x y\" 형식으로 입력하면 서버로 전송합니다. 예: 3 4\n");
		break;
	}

	case MessageType::MARKER_PLACED: {
		MarkerPlaced placed;
		if (!PacketCodec::Decode(packet, placed)) {
			printf("[오류] MARKER_PLACED 해석 실패\n");
			return;
		}
		printf("[수신] MARKER_PLACED: 플레이어 %u%s -> (%u, %u)\n",
			placed.senderId, player.IsMe(placed.senderId) ? "(나)" : "", placed.x, placed.y);
		break;
	}

	default:
		printf("[수신] 처리하지 않는 메시지 종류 %u (%zu바이트)\n", type, packet.size());
		break;
	}
}

// "x y" 입력을 좌표 패킷으로 전송
static void HandleInputLine(const char* line, NetworkClient& client, LocalPlayer& player)
{
	unsigned int x, y;
	if (sscanf(line, "%u %u", &x, &y) != 2) {
		printf("좌표는 \"x y\" 형식으로 입력하세요. 예: 3 4\n");
		return;
	}
	if (player.phase != LocalPlayer::Phase::Joined) {
		printf("아직 WELCOME을 받지 못했습니다.\n");
		return;
	}
	if (!SendMarker(client, player, x, y))
		printf("[오류] 좌표 전송 실패\n");
}

// 수신 큐의 바이트를 PacketBuffer에 넣고 완성된 패킷 처리
static bool ProcessReceived(NetworkClient& client, PacketBuffer& packetBuffer, LocalPlayer& player)
{
	std::vector<char> data;
	while (client.PollReceived(data))
		packetBuffer.Append(reinterpret_cast<const uint8_t*>(data.data()), data.size());

	PacketCodec::Packet packet;
	for (;;) {
		ExtractResult result = packetBuffer.TryExtractPacket(packet);
		if (result == ExtractResult::Ready) {
			HandlePacket(packet, player);
			continue;
		}
		return result != ExtractResult::InvalidLength;
	}
}

static bool ReadNickname(std::string& nickname)
{
	char line[64];
	for (;;) {
		printf("닉네임을 입력하세요: ");
		if (fgets(line, sizeof(line), stdin) == NULL) return false;

		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';

		if (len > 0) {
			nickname = line;
			return true;
		}
		printf("닉네임이 비어 있습니다.\n");
	}
}

// 게임 루프에서 로딩 갱신, 네트워크 수신, 콘솔 좌표 입력 처리
class ClientMode : public GameMode {
public:
	ClientMode(NetworkClient& client, LocalPlayer& player, PacketBuffer& packetBuffer)
		: m_client(client), m_player(player), m_packetBuffer(packetBuffer) {}

	void OnTimer(int timerId) override { GameWindow_OnTimer(timerId); }

	void Update(double frameTime) override
	{
		if (!ProcessReceived(m_client, m_packetBuffer, m_player)) {
			printf("[오류] 잘못된 길이의 패킷을 받아 연결을 끊습니다.\n");
			GameFramework::Quit();
			return;
		}
		if (!m_client.IsConnected()) {
			GameFramework::Quit();
			return;
		}

		ReadConsoleKeys();
	}

private:
	// 콘솔 창에서 한 줄씩 입력받아 "x y"는 좌표 전송, "q"는 종료
	void ReadConsoleKeys()
	{
		while (_kbhit()) {
			int c = _getch();
			if (c == 0 || c == 0xE0) {
				_getch();
				continue;
			}
			if (c == '\r') {
				printf("\n");
				m_line[m_lineLen] = '\0';
				if (strcmp(m_line, "q") == 0) {
					GameFramework::Quit();
					return;
				}
				if (m_lineLen > 0) HandleInputLine(m_line, m_client, m_player);
				m_lineLen = 0;
			}
			else if (c == '\b') {
				if (m_lineLen > 0) {
					m_lineLen--;
					printf("\b \b");
				}
			}
			else if (m_lineLen < (int)sizeof(m_line) - 1) {
				m_line[m_lineLen++] = (char)c;
				putchar(c);
			}
		}
	}

	NetworkClient& m_client;
	LocalPlayer& m_player;
	PacketBuffer& m_packetBuffer;
	char m_line[64] = { 0 };
	int m_lineLen = 0;
};

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

	// 1. 닉네임 입력 후 HELLO 전송
	LocalPlayer player;
	if (!ReadNickname(player.nickname)) {
		client.Disconnect();
		NetworkClient::Cleanup();
		return 1;
	}

	// 콘솔 입력은 코드 페이지 949라서 UTF-8로 변환 후 전송
	unsigned int inputCodePage = GetConsoleCP();
	if (inputCodePage == 0) inputCodePage = CP_ACP;

	Hello hello;
	if (!ToUtf8(player.nickname, inputCodePage, hello.name)) {
		printf("[오류] 닉네임을 UTF-8로 바꾸지 못했습니다. (입력 코드 페이지 %u)\n", inputCodePage);
		client.Disconnect();
		NetworkClient::Cleanup();
		return 1;
	}
	hello.nmaeLength = (uint16_t)hello.name.size();

	uint32_t helloId = player.IssueRequestId();
	if (!SendPacket(client, PacketCodec::Encode(hello, helloId))) {
		printf("[오류] HELLO 전송 실패\n");
		client.Disconnect();
		NetworkClient::Cleanup();
		return 1;
	}
	printf("[송신] HELLO 닉네임=%s (UTF-8 %zu바이트) 요청번호 %u\n", player.nickname.c_str(), hello.name.size(), helloId);
	printf("WELCOME을 기다리는 중... (창을 닫거나 콘솔에서 q 입력 시 종료)\n");

	// 2. 게임 창 실행
	HWND hWnd = GameWindow_Create(GetModuleHandle(NULL));
	if (hWnd == NULL) {
		printf("[오류] 게임 창을 만들지 못했습니다.\n");
		client.Disconnect();
		NetworkClient::Cleanup();
		return 1;
	}

	PacketBuffer packetBuffer;
	ClientMode mode(client, player, packetBuffer);
	GameFramework::Run(hWnd, &mode);

	if (IsWindow(hWnd)) DestroyWindow(hWnd);

	if (!client.IsConnected()) {
		ProcessReceived(client, packetBuffer, player);     // 끊기기 직전에 도착한 패킷 처리

		if (client.LastError() == 0)
			printf("서버가 연결을 종료했습니다.\n");
		else
			err_display(client.LastError());
	}

	client.Disconnect();
	NetworkClient::Cleanup();
	return 0;
}