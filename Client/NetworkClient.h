#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class NetworkClient {
public:
	NetworkClient() = default;
	~NetworkClient();

	NetworkClient(const NetworkClient&) = delete;
	NetworkClient& operator=(const NetworkClient&) = delete;

	static bool Startup();
	static void Cleanup();

	// 서버에 연결하고 네트워크 스레드를 시작
	bool Connect(const char* ip, unsigned short port);

	// 송신 큐에 남은 패킷을 모두 보내고 연결을 닫은 뒤, 네트워크 스레드가 끝날 때까지 기다린다.
	void Disconnect();

	// 바이트를 복사해 송신 큐에 넣는다.
	bool QueuePacket(const char* data, int length);

	// 수신 큐에서 받은 바이트 덩어리 하나를 꺼낸다. 없으면 false. 안 기다림
	bool PollReceived(std::vector<char>& out);

	bool IsConnected() const { return m_connected && !m_closing; }

	// 마지막 오류 코드
	int LastError() const { return m_lastError; }

private:
	void NetworkThread();
	bool HasPendingSend();
	bool FlushSend();
	bool ReceiveOnce(bool& peerClosed);
	size_t ReceivedBytes();

	static const size_t MAX_SEND_QUEUED_BYTES = 8 * 1024 * 1024;
	static const size_t MAX_RECV_QUEUED_BYTES = 8 * 1024 * 1024;

	SOCKET              m_sock = INVALID_SOCKET;
	std::thread         m_networkThread;

	std::atomic<bool>   m_connected{ false };
	std::atomic<bool>   m_closing{ false };
	std::atomic<int>    m_lastError{ 0 };

	std::mutex                      m_sendMutex;
	std::deque<std::vector<char>>   m_sendQueue;
	size_t                          m_sendQueuedBytes = 0;

	// 보내는 중인 패킷과 이미 보낸 바이트 수
	std::vector<char>               m_sending;
	size_t                          m_sendOffset = 0;

	std::mutex                      m_recvMutex;
	std::deque<std::vector<char>>   m_recvQueue;
	size_t                          m_recvQueuedBytes = 0;
};