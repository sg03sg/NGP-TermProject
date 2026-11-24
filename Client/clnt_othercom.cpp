#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include "NetworkClient.h"
#include "EncodingUtil.h"
#include "LocalPlayer.h"
#include "../PacketCodec.h"
#include "../PacketBuffer.h"
#include "../GameFramework.h"
#include "ClientState.h"
#include "LobbyScreen.h"
#include "InputController.h"
#include "GameWindow.h"

#include <sstream>
#include <conio.h>
#include <string>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

// 로비·파티 상태와 화면은 LobbyScreen이 소유하고 여기서는 가리키기만 한다.
static LobbyScreen* lobby = nullptr;

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
	// PARTY_STATE, LEAVE_APPROVED, REQUEST_REJECT는 LobbyScreen이 상태에 반영하고 화면을 갱신
	if (lobby->HandlePacket(packet)) return;

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
		lobby->OnWelcome(welcome.playerId);
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
	case MessageType::BATTLE_INIT: {
		BattleInit message;

		if (!PacketCodec::Decode(packet, message)) {
			printf("[오류] BATTLE_INIT 해석 실패\n");
			return;
		}

		// 현재 참가한 파티의 전투인지 확인
		if (!lobby->State().InParty() ||
			message.partyId != lobby->State().Party().partyId) {
			printf("[오류] 현재 파티와 다른 BATTLE_INIT\n");
			return;
		}

		// 클라이언트 상태를 전투로 바꾸고 게임 화면을 해당 스테이지의 전투 화면으로 전환
		lobby->OnBattleInit(message);
		GameWindow_EnterBattle(message, lobby->State().MyPlayerId());

		printf("[수신] BATTLE_INIT partyId=%u mapId=%u time=%u\n",
			message.partyId,
			static_cast<unsigned>(message.mapId),
			static_cast<unsigned>(message.remainingSeconds));

		for (const auto& playerInfo : message.players) {
			printf("  playerId=%u mana=%u/%u\n",
				playerInfo.playerId,
				static_cast<unsigned>(playerInfo.mana),
				static_cast<unsigned>(playerInfo.maxMana));
		}

		for (const auto& structure : message.structures) {
			printf("  structureId=%u position=(%u, %u) hp=%u/%u\n",
				structure.structureId,
				structure.x, structure.y,
				structure.hp, structure.maxHp);
		}
		break;
	}

	case MessageType::UNIT_CREATED: {
		UnitCreated message;

		if (!PacketCodec::Decode(packet, message)) {
			printf("[오류] UNIT_CREATED 해석 실패\n");
			return;
		}

		printf(
			"[수신] UNIT_CREATED unitId=%u ownerId=%u "
			"type=%u position=(%u, %u) hp=%u/%u\n",
			message.unitId,
			message.ownerId,
			static_cast<unsigned>(message.unitType),
			message.x,
			message.y,
			message.hp,
			message.maxHp);

		// 같은 파티 전체에 오는 생성 이벤트를 전투 화면의 병력으로 반영
		GameWindow_ApplyUnitCreated(message);
		break;
	}

	case MessageType::MANA_STATE: {
		ManaState message;

		if (!PacketCodec::Decode(packet, message)) {
			printf("[오류] MANA_STATE 해석 실패\n");
			return;
		}

		printf("[수신] MANA_STATE mana=%u/%u\n",
			static_cast<unsigned>(message.mana),
			static_cast<unsigned>(message.maxMana));

		GameWindow_ApplyManaState(message);
		break;
	}

	case MessageType::BATTLE_STATE: {
		BattleState message;

		if (!PacketCodec::Decode(packet, message)) {
			printf("[오류] BATTLE_STATE 해석 실패\n");
			return;
		}

		if (!lobby || lobby->State().Screen() != ScreenState::Battle ||	message.partyId != lobby->State().Battle().partyId) 
		{
			return;
		}

		printf(
			"[수신] BATTLE_STATE partyId=%u tick=%u units=%zu\n",
			message.partyId,
			message.serverTick,
			message.units.size());

		for (const UnitStateInfo& unit : message.units) {
			printf(
				"  unitId=%u position=(%u, %u) hp=%u/%u\n",
				unit.unitId,
				unit.x,
				unit.y,
				unit.hp,
				unit.maxHp);
		}
		GameWindow_ApplyBattleState(message);
		break;
	}

	case MessageType::BATTLE_PROGRESS: {
		BattleProgress progress{};
		if (!PacketCodec::Decode(packet, progress)) {
			printf("[오류] BATTLE_PROGRESS 해석 실패\n");
			return;
		}

		if (!lobby || lobby->State().Screen() != ScreenState::Battle ||
			progress.partyId != lobby->State().Battle().partyId) {
			return;
		}

		GameWindow_ApplyBattleProgress(progress);
		break;
	}

	case MessageType::BATTLE_RESULT: {
		BattleResult result{};
		if (!PacketCodec::Decode(packet, result)) {
			printf("[오류] BATTLE_RESULT 해석 실패\n");
			return;
		}

		if (!lobby || lobby->State().Screen() != ScreenState::Battle ||
			result.partyId != lobby->State().Battle().partyId) {
			return;
		}

		printf("[수신] BATTLE_RESULT partyId=%u reason=%u stars=%u\n",
			result.partyId,
			static_cast<unsigned>(result.reason),
			static_cast<unsigned>(result.stars));

		lobby->OnBattleResult();
		GameWindow_ApplyBattleResult(result);
		break;
	}

	default:
		printf("[수신] 처리하지 않는 메시지 종류 %u (%zu바이트)\n", type, packet.size());
		break;
	}
}

static void HandleInputLine(
	const char* line,
	NetworkClient& client,
	LocalPlayer& player)
{
	if (player.phase != LocalPlayer::Phase::Joined) {
		printf("아직 WELCOME을 받지 못했습니다.\n");
		return;
	}

	std::istringstream input(line);
	std::string command;
	input >> command;

	if (command == "join") {
		int mapId;
		std::string extra;

		if (!(input >> mapId) || (input >> extra) ||
			mapId < 1 || mapId > kStageCount) {
			printf("사용법: join 1 ~ join %u\n",
				static_cast<unsigned>(kStageCount));
			return;
		}

		if (lobby->State().InParty()) {
			printf("이미 파티에 참가 중입니다.\n");
			return;
		}

		JoinMap message;
		message.mapId = static_cast<uint8_t>(mapId);

		const uint32_t requestId = player.IssueRequestId();

		if (SendPacket(client, PacketCodec::Encode(message, requestId))) {
			printf("[송신] JOIN_MAP mapId=%d requestId=%u\n",
				mapId, requestId);
		}
		else {
			printf("[오류] JOIN_MAP 전송 실패\n");
		}
		return;
	}

	if (command == "start") {
		std::string extra;
		if (input >> extra) {
			printf("사용법: start\n");
			return;
		}

		if (lobby->State().Screen() != ScreenState::PartyWaiting ||
			!lobby->State().IsPartyLeader()) {
			printf("대기 중인 파티의 방장만 시작할 수 있습니다.\n");
			return;
		}

		StartGame message;
		message.partyId = lobby->State().Party().partyId;

		const uint32_t requestId = player.IssueRequestId();

		if (SendPacket(client, PacketCodec::Encode(message, requestId))) {
			printf("[송신] START_GAME partyId=%u requestId=%u\n",
				message.partyId, requestId);
		}
		else {
			printf("[오류] START_GAME 전송 실패\n");
		}
		return;
	}

	if (command == "spawn") {
		if (lobby->State().Screen() != ScreenState::Battle) {
			printf("전투를 시작한 뒤 소환할 수 있습니다.\n");
			return;
		}

		int unitType;
		long long x, y;
		std::string extra;

		if (!(input >> unitType >> x >> y) ||
			(input >> extra) ||
			unitType < 0 || unitType > 2 ||
			x < 0 || y < 0 ||
			x > INT32_MAX || y > INT32_MAX) {
			printf("사용법: spawn 병종 x y\n");
			printf("병종: 0=바바리안, 1=아처, 2=자이언트\n");
			return;
		}

		SpawnRequest message;
		message.unitType = static_cast<uint8_t>(unitType);
		message.x = static_cast<uint32_t>(x);
		message.y = static_cast<uint32_t>(y);

		const uint32_t requestId = player.IssueRequestId();

		if (SendPacket(
			client,
			PacketCodec::Encode(message, requestId))) {
			printf(
				"[송신] SPAWN_REQUEST type=%d position=(%u, %u) "
				"requestId=%u\n",
				unitType, message.x, message.y, requestId);
		}
		else {
			printf("[오류] SPAWN_REQUEST 전송 실패\n");
		}

		return;
	}

	// 기존 좌표 마커 테스트
	std::istringstream coordinates(line);
	long long x, y;
	std::string extra;

	if (!(coordinates >> x >> y) || (coordinates >> extra) ||
		x < 0 || y < 0 || x > UINT32_MAX || y > UINT32_MAX) {
		printf("명령: join 1 / start / x y / q\n");
		return;
	}

	if (!SendMarker(client, player,
		static_cast<uint32_t>(x), static_cast<uint32_t>(y))) {
		printf("[오류] 좌표 전송 실패\n");
	}
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

		// 기존 콘솔 명령도 계속 사용 가능
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

	// 로비 화면과 입력 처리는 서버로 보내는 방법과 요청 번호 발급기를 받아 동작
	auto sendToServer = [&client](const PacketCodec::Packet& packet) { return SendPacket(client, packet); };
	auto nextRequestId = [&player]() { return player.IssueRequestId(); };

	LobbyScreen lobbyScreen(sendToServer, nextRequestId);
	lobbyScreen.SetWindow(hWnd);
	lobby = &lobbyScreen;
	GameWindow_SetLobby(&lobbyScreen);

	// 창의 클릭은 InputController가 받아 로비, 파티 UI, 전투 소환으로 분배
	InputController inputController(lobbyScreen, sendToServer, nextRequestId);
	GameWindow_SetInput(&inputController);

	PacketBuffer packetBuffer;
	ClientMode mode(client, player, packetBuffer);
	GameFramework::Run(hWnd, &mode);

	GameWindow_SetInput(nullptr);
	GameWindow_SetLobby(nullptr);
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