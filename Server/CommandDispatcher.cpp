#include "CommandDispatcher.h"
#include "GameServer.h"
#include "Protocol.h"
#include "MapCatalog.h"
#include <iostream>

namespace {
    // 패킷 헤더의 messageType
    uint16_t MessageTypeOf(const std::vector<uint8_t>& packet)
    {
        if (packet.size() < PACKET_HEADER_SIZE) return 0;
        return static_cast<uint16_t>((packet[4] << 8) | packet[5]);
    }

    // 처리 함수를 구현하기 전까지 수신 사실만 로그로 남김
    void LogReceived(const Command& cmd, const char* name)
    {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": " << name
            << " 수신 " << cmd.packet.size() << "바이트, 처리 함수 미구현" << std::endl;
    }
}

CommandDispatcher::CommandDispatcher(GameServer& server)
    : server_(server),
    partyHandler_(PartyCommandHandler::Context{
        server.GetPartyManager(),
        [this](PlayerId pid, const PacketCodec::Packet& p) { SendToPlayer(pid, p); },
        [this](const std::vector<PlayerId>& pids, const PacketCodec::Packet& p) { SendToPlayers(pids, p); },
        [this](PlayerId pid) { auto it = players_.find(pid); return it != players_.end() ? it->second : std::string(); },
        [&server]() { server.GetLobbyBroadcaster().Invalidate(); } })
{
}

void CommandDispatcher::SendToSession(int sessionId, const PacketCodec::Packet& packet)
{
    SendToSessions({ sessionId }, packet);
}

void CommandDispatcher::SendToSessions(const std::vector<int>& sessionIds, const PacketCodec::Packet& packet)
{
    if (packet.empty() || sessionIds.empty()) return;
    server_.GetSessionManager().SendToSessions(sessionIds, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()));
}

void CommandDispatcher::SendToPlayer(PlayerId pid, const PacketCodec::Packet& packet)
{
    auto it = playerToSession_.find(pid);
    if (it != playerToSession_.end()) SendToSession(it->second, packet);
}

void CommandDispatcher::SendToPlayers(const std::vector<PlayerId>& pids, const PacketCodec::Packet& packet)
{
    std::vector<int> ids;
    for (PlayerId pid : pids) {
        auto it = playerToSession_.find(pid);
        if (it != playerToSession_.end()) ids.push_back(it->second);
    }
    SendToSessions(ids, packet);
}

void CommandDispatcher::SendBattleState(const BattleState& message)
{
    const Party* party = server_.GetPartyManager().GetParty(message.partyId);

    if (!party || party->Status() != PartyStatus::Running) 
    {
        return;
    }

    const PacketCodec::Packet packet = PacketCodec::Encode(message, 0);

    if (packet.empty()) 
    {
        std::cout << "[BATTLE_STATE] Encode failed\n";
        return;
    }

    // 전체 접속자가 아니라 해당 파티원에게만 전송
    SendToPlayers(party->Members(), packet);
}

PlayerId CommandDispatcher::PlayerOf(int sessionId) const
{
    auto it = sessionToPlayer_.find(sessionId);
    return it == sessionToPlayer_.end() ? kInvalidPlayerId : it->second;
}

void CommandDispatcher::Dispatch(const Command& cmd)
{
    switch (cmd.type) {
    case Command::Type::Packet:
        DispatchPacket(cmd);
        break;
    case Command::Type::Disconnected:
        HandleDisconnected(cmd.sessionId);
        break;
    }
}

void CommandDispatcher::DispatchPacket(const Command& cmd)
{
    const uint16_t type = MessageTypeOf(cmd.packet);

    switch (static_cast<MessageType>(type)) {
    case MessageType::HELLO:            HandleHello(cmd);           break;
    case MessageType::JOIN_MAP:         HandleJoinMap(cmd);         break;
    case MessageType::START_GAME:       HandleStartGame(cmd);       break;
    case MessageType::LEAVE_PARTY:      HandleLeaveParty(cmd);      break;
    case MessageType::CHAT_SEND:        HandleChatSend(cmd);        break;
    case MessageType::SPAWN_REQUEST:    HandleSpawn(cmd);           break;
    case MessageType::ABORT_VOTE_CAST:  HandleAbortVoteCast(cmd);   break;
    case MessageType::MARKER_REQUEST:   HandleMarkerRequest(cmd);   break;

    default:
        // 서버가 보내는 종류이거나 정의되지 않은 종류
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": 처리하지 않는 메시지 종류 "
            << type << " " << cmd.packet.size() << "바이트" << std::endl;
        break;
    }
}

void CommandDispatcher::HandleHello(const Command& cmd)
{
    Hello hello;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, hello, &header)) {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 디코딩 실패" << std::endl;
        return;
    }
    if (PlayerOf(cmd.sessionId) != kInvalidPlayerId) return;   // 중복 HELLO 무시

    const PlayerId pid = nextPlayerId_++;
    sessionToPlayer_[cmd.sessionId] = pid;
    playerToSession_[pid] = cmd.sessionId;
    players_[pid] = hello.name;
    std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO \"" << hello.name << "\" -> 플레이어 " << pid << std::endl;

    Welcome welcome;
    welcome.playerId = pid;
    SendToSession(cmd.sessionId, PacketCodec::Encode(welcome, header.requestId));
    server_.GetLobbyBroadcaster().Invalidate();
}

void CommandDispatcher::HandleJoinMap(const Command& cmd)
{
    const PlayerId pid = PlayerOf(cmd.sessionId);
    if (pid == kInvalidPlayerId) { std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 전 JOIN_MAP 무시" << std::endl; return; }
    partyHandler_.HandleJoinMap(cmd, pid);
}

void CommandDispatcher::HandleLeaveParty(const Command& cmd)
{
    const PlayerId pid = PlayerOf(cmd.sessionId);
    if (pid == kInvalidPlayerId) { std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": HELLO 전 LEAVE_PARTY 무시" << std::endl; return; }
    partyHandler_.HandleLeaveParty(cmd, pid);
}
void CommandDispatcher::HandleChatSend(const Command& cmd) { LogReceived(cmd, "CHAT_SEND"); }

void CommandDispatcher::HandleSpawn(const Command& cmd) {
    SpawnRequest request;
    PacketHeader header;

    if (!PacketCodec::Decode(cmd.packet, request, &header)) 
    {
        std::cout << "[SPAWN_REQUEST] Decode failed\n";
        return;
    }

    auto reject = [&](ErrorCode error) {
        RequestRejected message;

        message.rejectedType =
            static_cast<uint16_t>(MessageType::SPAWN_REQUEST);
        message.errorCode = static_cast<uint16_t>(error);

        SendToSession(cmd.sessionId, PacketCodec::Encode(message, header.requestId));
        };

    // 소유자는 요청 본문이 아니라 접속 세션으로 확인한다.
    const PlayerId pid = PlayerOf(cmd.sessionId);
    PartyManager& parties = server_.GetPartyManager();

    if (pid == kInvalidPlayerId || !parties.IsInParty(pid)) 
    {
        reject(ErrorCode::NotInBattle);
        return;
    }

    const PartyId partyId = parties.PartyOf(pid);
    const Party* party = parties.GetParty(partyId);
    BattleInstance* battle = server_.FindBattle(partyId);

    if (!party || party->Status() != PartyStatus::Running || !battle)
    {
        reject(ErrorCode::NotInBattle);
        return;
    }

    // 패킷 좌표는 uint32_t이고 Unit 좌표는 int이므로 가능한 범위 검사 
    if (request.x > static_cast<uint32_t>(INT_MAX) || request.y > static_cast<uint32_t>(INT_MAX))
    {
        reject(ErrorCode::InvalidPosition);
        return;
    }

    Position position{
        static_cast<int32_t>(request.x),
        static_cast<int32_t>(request.y)
    };

    const Result result = battle->TrySpawnUnit(
        pid,
        static_cast<UnitType>(request.unitType),
        position);

    if (!result.success) {
        reject(result.error);
        return;
    }

    // 병종별 정렬 삽입이므로 반환된 ID로 조회한다.
    Unit* unit = battle->FindUnit(static_cast<int>(result.unitId));

    if (!unit) 
    {
        std::cout << "[SPAWN_REQUEST] Created unit not found\n";
        return;
    }

    UnitCreated created;
    created.unitId = static_cast<uint32_t>(unit->GetId());
    created.ownerId = pid;
    created.unitType = static_cast<uint8_t>(unit->GetType());
    created.x = static_cast<uint32_t>(unit->x);
    created.y = static_cast<uint32_t>(unit->y);
    created.hp = static_cast<uint32_t>(unit->GetHp());
    created.maxHp = static_cast<uint32_t>(unit->GetMaxHp());

    // 생성 이벤트는 요청자를 포함한 같은 파티 전체에 전송
    // 공통 이벤트이므로 requestId는 0을 사용한다.
    SendToPlayers(party->Members(), PacketCodec::Encode(created, 0));

    // 차감된 마나는 요청한 사람에게만 전송
    const PlayerState* player = battle->FindPlayer(pid);
    if (player != nullptr) {
        ManaState mana;
        mana.mana = player->mana;
        mana.maxMana = player->maxMana;
        SendToSession(cmd.sessionId, PacketCodec::Encode(mana, header.requestId));
    }

    std::cout
        << "[UNIT_CREATED] partyId=" << partyId
        << " unitId=" << created.unitId
        << " ownerId=" << created.ownerId
        << " type=" << static_cast<unsigned>(created.unitType)
        << " position=(" << created.x << ", " << created.y << ")"
        << '\n';
}

void CommandDispatcher::HandleAbortVoteCast(const Command& cmd) { LogReceived(cmd, "ABORT_VOTE_CAST"); }
void CommandDispatcher::HandleMarkerRequest(const Command& cmd)
{
    MarkerRequest req;
    PacketHeader header;
    if (!PacketCodec::Decode(cmd.packet, req, &header)) {
        std::cout << "[CommandDispatcher] 세션 " << cmd.sessionId << ": MARKER_REQUEST 디코딩 실패" << std::endl;
        return;
    }

    // HELLO 전이면 세션 번호를 발신자로 쓴다 (통합 테스트용)
    const PlayerId pid = PlayerOf(cmd.sessionId);
    MarkerPlaced placed;
    placed.senderId = pid != kInvalidPlayerId ? pid : static_cast<uint32_t>(cmd.sessionId);
    placed.x = req.x;
    placed.y = req.y;
    const PacketCodec::Packet packet = PacketCodec::Encode(placed, 0);

    // 파티가 있으면 파티원 전체, 없으면 접속 중인 모두에게 보낸다 (보낸 사람 포함)
    PartyManager& parties = server_.GetPartyManager();
    if (pid != kInvalidPlayerId && parties.IsInParty(pid)) {
        SendToPlayers(parties.MembersOf(parties.PartyOf(pid)), packet);
    }
    else {
        SendToSessions(server_.GetSessionManager().GetSessionIds(), packet);
    }
}

void CommandDispatcher::HandleStartGame(const Command& cmd)
{
    StartGame request;
    PacketHeader header;

    if (!PacketCodec::Decode(cmd.packet, request, &header)) {
        std::cout << "[START_GAME] Decode failed\n";
        return;
    }

    auto reject = [&](PartyError error) {
        RequestRejected message;
        message.rejectedType =
            static_cast<uint16_t>(MessageType::START_GAME);
        message.errorCode = static_cast<uint16_t>(error);

        SendToSession(
            cmd.sessionId,
            PacketCodec::Encode(message, header.requestId));
        };

    const PlayerId pid = PlayerOf(cmd.sessionId);
    PartyManager& parties = server_.GetPartyManager();

    // 클라이언트가 보낸 partyId를 서버 소속 정보와 비교한다.
    if (pid == kInvalidPlayerId || !parties.IsInParty(pid) || parties.PartyOf(pid) != request.partyId) {
        reject(PartyError::NotInParty);
        return;
    }

    Party* party = parties.GetParty(request.partyId);
    if (!party) {
        reject(PartyError::NotInParty);
        return;
    }

    if (party->Leader() != pid) {
        reject(PartyError::NotLeader);
        return;
    }

    if (!party->CanStart(pid) ||
        server_.FindBattle(request.partyId) != nullptr) {
        reject(PartyError::StageLocked);
        return;
    }

    // 확정된 인원에 맞는 스테이지 맵 데이터 생성
    MapData map;
    if (!BuildMapData(static_cast<MapDataId>(party->GetMapId()), party->Size(), map)) {
        reject(PartyError::InvalidStage);
        return;
    }

    auto battle = std::make_unique<BattleInstance>(party->Id());

    if (!battle->Initialize(map, party->Members())) {
        std::cout << "[START_GAME] Battle initialization failed\n";
        reject(PartyError::InvalidStage);
        return;
    }

    const BattleInit init = battle->BuildBattleInit();
    const PacketCodec::Packet packet =
        PacketCodec::Encode(init, header.requestId);

    if (packet.empty()) {
        std::cout << "[START_GAME] BATTLE_INIT encoding failed\n";
        return;
    }

    if (!server_.AddBattle(party->Id(), std::move(battle))) {
        reject(PartyError::StageLocked);
        return;
    }

    // 초기화와 보관이 끝난 뒤 시작 상태로 변경한다.
    party->SetStatus(PartyStatus::Running);
    server_.GetLobbyBroadcaster().Invalidate();

    SendToPlayers(party->Members(), packet);

    std::cout << "[START_GAME] partyId=" << party->Id()
        << " members=" << party->Size()
        << " battle initialized\n";
}

void CommandDispatcher::HandleDisconnected(int sessionId)
{
    std::cout << "[CommandDispatcher] 세션 " << sessionId << ": 연결 종료 이벤트 수신" << std::endl;

    auto it = sessionToPlayer_.find(sessionId);
    if (it == sessionToPlayer_.end()) return;   // HELLO 전에 끊김
    const PlayerId pid = it->second;

    partyHandler_.HandlePlayerLeft(pid);        // 파티에서 제거, 남은 파티원에게 PARTY_STATE
    sessionToPlayer_.erase(it);
    playerToSession_.erase(pid);
    players_.erase(pid);
    server_.GetLobbyBroadcaster().Invalidate();
}