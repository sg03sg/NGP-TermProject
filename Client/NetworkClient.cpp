#include "NetworkClient.h"

#pragma comment(lib, "ws2_32")

namespace {
	const int SEND_TIMEOUT_MS = 3000;
}

NetworkClient::~NetworkClient() {
	Disconnect();
}

bool NetworkClient::Startup() {
	WSADATA wsa;
	return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

void NetworkClient::Cleanup() {
	WSACleanup();
}

bool NetworkClient::Connect(const char* ip, unsigned short port) {
	m_lastError = 0;

	if (m_connected && !m_closing) {
		m_lastError = WSAEISCONN;
		return false;
	}
	Disconnect();   // 이전 연결이 끊긴 채 남아 있으면 정리

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (ip == nullptr || inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
		m_lastError = WSAEINVAL;
		return false;
	}

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) {
		m_lastError = WSAGetLastError();
		return false;
	}

	BOOL noDelay = TRUE;
	setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)&noDelay, sizeof(noDelay));

	// 서버가 읽지 않아 send()가 영원히 막히는 일을 막음
	int sendTimeout = SEND_TIMEOUT_MS;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char*)&sendTimeout, sizeof(sendTimeout));

	if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		m_lastError = WSAGetLastError();
		closesocket(s);
		return false;
	}

	m_sock = s;
	m_closing = false;
	m_connected = true;
	m_sendThread = std::thread(&NetworkClient::SendThread, this);
	return true;
}

void NetworkClient::Disconnect() {
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_closing = true;
	}
	m_queueCv.notify_all();

	// 송신 스레드는 큐에 남은 패킷을 다 보내고 끝낸다.
	if (m_sendThread.joinable())
		m_sendThread.join();

	if (m_sock != INVALID_SOCKET) {
		shutdown(m_sock, SD_SEND);
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_queue.clear();
		m_queuedBytes = 0;
	}
	m_connected = false;
}

bool NetworkClient::QueuePacket(const char* data, int length) {
	if (data == nullptr || length <= 0) return false;

	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		if (!m_connected || m_closing) return false;
		if (m_queuedBytes + (size_t)length > MAX_QUEUED_BYTES) return false;

		m_queue.emplace_back(data, data + length);
		m_queuedBytes += (size_t)length;
	}
	m_queueCv.notify_one();
	return true;
}

void NetworkClient::SendThread() {
	for (;;) {
		std::vector<char> packet;
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);
			m_queueCv.wait(lock, [this]() { return !m_queue.empty() || m_closing; });
			if (m_queue.empty()) break;      // 보낼게 없고 끊는 중일때

			packet = std::move(m_queue.front());
			m_queue.pop_front();
			m_queuedBytes -= packet.size();
		}

		// send()는 요청한 만큼 모두 보내지 못할 수 있으므로 오프셋을 유지하며 반복
		size_t offset = 0;
		while (offset < packet.size()) {
			int sent = send(m_sock, packet.data() + offset, (int)(packet.size() - offset), 0);
			if (sent == SOCKET_ERROR) {
				m_lastError = WSAGetLastError();
				{
					std::lock_guard<std::mutex> lock(m_queueMutex);
					m_connected = false;
					m_queue.clear();
					m_queuedBytes = 0;
				}
				return;
			}
			offset += (size_t)sent;
		}
	}
}