#include "NetworkClient.h"

#include <chrono>
#include <cstring>

#pragma comment(lib, "ws2_32")

namespace {
	// select 한 번의 대기 시간.
	const int SELECT_TICK_MS = 10;

	// Disconnect() 후 남은 패킷을 보내는 데 쓰는 최대 시간
	const int CLOSE_WAIT_MS = 3000;

	const int RECV_CHUNK = 8192;

	using Clock = std::chrono::steady_clock;
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
	Disconnect();   // 이전 연결이 끊긴 채 남아 있으면 스레드와 소켓을 정리한다

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

	if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		m_lastError = WSAGetLastError();
		closesocket(s);
		return false;
	}

	// 연결된 뒤에는 논블로킹으로 바꾼다.
	u_long nonBlocking = 1;
	if (ioctlsocket(s, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
		m_lastError = WSAGetLastError();
		closesocket(s);
		return false;
	}

	{
		std::lock_guard<std::mutex> lock(m_recvMutex);
		m_recvQueue.clear();
		m_recvQueuedBytes = 0;
	}
	m_sending.clear();
	m_sendOffset = 0;

	m_sock = s;
	m_closing = false;
	m_connected = true;
	m_networkThread = std::thread(&NetworkClient::NetworkThread, this);
	return true;
}

void NetworkClient::Disconnect() {
	m_closing = true;

	if (m_networkThread.joinable())
		m_networkThread.join();

	if (m_sock != INVALID_SOCKET) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	{
		std::lock_guard<std::mutex> lock(m_sendMutex);
		m_sendQueue.clear();
		m_sendQueuedBytes = 0;
	}
	m_sending.clear();
	m_sendOffset = 0;
	m_connected = false;
}

bool NetworkClient::QueuePacket(const char* data, int length) {
	if (data == nullptr || length <= 0) return false;

	std::lock_guard<std::mutex> lock(m_sendMutex);
	if (!m_connected || m_closing) return false;
	if (m_sendQueuedBytes + (size_t)length > MAX_SEND_QUEUED_BYTES) return false;

	m_sendQueue.emplace_back(data, data + length);
	m_sendQueuedBytes += (size_t)length;
	return true;
}

bool NetworkClient::PollReceived(std::vector<char>& out) {
	std::lock_guard<std::mutex> lock(m_recvMutex);
	if (m_recvQueue.empty()) return false;

	out = std::move(m_recvQueue.front());
	m_recvQueue.pop_front();
	m_recvQueuedBytes -= out.size();
	return true;
}

bool NetworkClient::HasPendingSend() {
	if (m_sendOffset < m_sending.size()) return true;
	std::lock_guard<std::mutex> lock(m_sendMutex);
	return !m_sendQueue.empty();
}

size_t NetworkClient::ReceivedBytes() {
	std::lock_guard<std::mutex> lock(m_recvMutex);
	return m_recvQueuedBytes;
}

// 쓰기 가능할 때 호출된다. 보낼 수 있는 만큼 보내고 다음 차례에 이어서 보낸다.
bool NetworkClient::FlushSend() {
	for (;;) {
		if (m_sendOffset >= m_sending.size()) {
			std::lock_guard<std::mutex> lock(m_sendMutex);
			if (m_sendQueue.empty()) {
				m_sending.clear();
				m_sendOffset = 0;
				return true;
			}
			m_sending = std::move(m_sendQueue.front());
			m_sendQueue.pop_front();
			m_sendQueuedBytes -= m_sending.size();
			m_sendOffset = 0;
		}

		int sent = send(m_sock, m_sending.data() + m_sendOffset, (int)(m_sending.size() - m_sendOffset), 0);
		if (sent == SOCKET_ERROR) {
			int e = WSAGetLastError();
			if (e == WSAEWOULDBLOCK) return true;     // 송신 버퍼가 찼다.
			m_lastError = e;
			return false;
		}
		m_sendOffset += (size_t)sent;
	}
}

// 읽기 가능할 때 호출된다. 한 번 recv해서 받은 바이트를 수신 큐에 넣는다.
bool NetworkClient::ReceiveOnce(bool& peerClosed) {
	char buf[RECV_CHUNK];
	int received = recv(m_sock, buf, RECV_CHUNK, 0);

	if (received > 0) {
		std::lock_guard<std::mutex> lock(m_recvMutex);
		m_recvQueue.emplace_back(buf, buf + received);
		m_recvQueuedBytes += (size_t)received;
		return true;
	}
	if (received == 0) {            // 서버가 FIN을 보냈다 = 정상 종료
		peerClosed = true;
		return true;
	}

	int e = WSAGetLastError();
	if (e == WSAEWOULDBLOCK) return true;
	m_lastError = e;
	return false;
}

void NetworkClient::NetworkThread() {
	bool closeDeadlineSet = false;
	Clock::time_point closeDeadline;

	for (;;) {
		bool pendingSend = HasPendingSend();

		// 종료 요청: 남은 패킷을 다 보냈으면 FIN을 보내고 끝낸다. 제한 시간이 지나도 끝낸다.
		if (m_closing) {
			if (!closeDeadlineSet) {
				closeDeadline = Clock::now() + std::chrono::milliseconds(CLOSE_WAIT_MS);
				closeDeadlineSet = true;
			}
			if (!pendingSend) {
				shutdown(m_sock, SD_SEND);
				break;
			}
			if (Clock::now() >= closeDeadline) break;
		}

		// 수신 큐가 가득 차 있으면 읽지 않는다. 메인 스레드가 꺼내 가면 다시 읽는다.
		bool watchRead = ReceivedBytes() < MAX_RECV_QUEUED_BYTES;
		bool watchWrite = pendingSend;

		if (!watchRead && !watchWrite) {
			Sleep(SELECT_TICK_MS);
			continue;
		}

		fd_set readSet, writeSet;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		if (watchRead)  FD_SET(m_sock, &readSet);
		if (watchWrite) FD_SET(m_sock, &writeSet);

		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = SELECT_TICK_MS * 1000;

		int n = select(0, watchRead ? &readSet : nullptr, watchWrite ? &writeSet : nullptr, nullptr, &tv);
		if (n == SOCKET_ERROR) {
			m_lastError = WSAGetLastError();
			break;
		}
		if (n == 0) continue;

		if (watchWrite && FD_ISSET(m_sock, &writeSet)) {
			if (!FlushSend()) break;
		}

		if (watchRead && FD_ISSET(m_sock, &readSet)) {
			bool peerClosed = false;
			if (!ReceiveOnce(peerClosed)) break;
			if (peerClosed) break;
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_sendMutex);
		m_connected = false;
		m_sendQueue.clear();
		m_sendQueuedBytes = 0;
	}
}