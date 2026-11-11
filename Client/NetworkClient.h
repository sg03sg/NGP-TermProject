#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <condition_variable>
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

	// 서버에 연결하고 송신 스레드를 시작한다. 
	bool Connect(const char* ip, unsigned short port);

	// 큐에 남은 패킷을 모두 보낸 뒤 연결을 닫고 네트워크 스레드를 기다림
	void Disconnect();

	// 패킷 바이트를 복사해 송신 큐에 넣음
	bool QueuePacket(const char* data, int length);

	bool IsConnected() const { return m_connected && !m_closing; }
	int LastError() const { return m_lastError; }

private:
	void SendThread();

	static const size_t MAX_QUEUED_BYTES = 8 * 1024 * 1024;

	SOCKET              m_sock = INVALID_SOCKET;
	std::thread         m_sendThread;

	std::atomic<bool>   m_connected{ false };
	std::atomic<bool>   m_closing{ false };
	std::atomic<int>    m_lastError{ 0 };

	std::mutex                      m_queueMutex;
	std::condition_variable         m_queueCv;
	std::deque<std::vector<char>>   m_queue;
	size_t                          m_queuedBytes = 0;
};