// Runs the real libcurl transport against a loopback HTTP server.

#include "ncei/cdo_client.hpp"
#include "ncei/data_service_client.hpp"
#include "ncei/http_client.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <gtest/gtest.h>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

#ifdef _WIN32
using Socket = SOCKET;
using AddressLength = int;
constexpr Socket kInvalidSocket = INVALID_SOCKET;
constexpr int kShutdownBoth = SD_BOTH;
void close_socket(Socket socket) {
	(void)::closesocket(socket);
}
void initialize_sockets() {
	struct Winsock {
		Winsock() {
			WSADATA data{};
			if (::WSAStartup(MAKEWORD(2, 2), &data) != 0) {
				throw std::runtime_error("WSAStartup() failed");
			}
		}
		~Winsock() { (void)::WSACleanup(); }
	};
	static const Winsock initialized;
}
#else
using Socket = int;
using AddressLength = socklen_t;
constexpr Socket kInvalidSocket = -1;
constexpr int kShutdownBoth = SHUT_RDWR;
void close_socket(Socket socket) {
	(void)::close(socket);
}
void initialize_sockets() {}
#endif
using SocketIoResult = decltype(::recv(Socket{}, nullptr, 0, 0));

class NoProxyEnvironment : public testing::Environment {
	void SetUp() override {
#ifdef _WIN32
		(void)::_putenv_s("NO_PROXY", "127.0.0.1");
		(void)::_putenv_s("no_proxy", "127.0.0.1");
#else
		(void)::setenv("NO_PROXY", "127.0.0.1", 1);
		(void)::setenv("no_proxy", "127.0.0.1", 1);
#endif
	}
};
[[maybe_unused]] testing::Environment* const kNoProxy =
	testing::AddGlobalTestEnvironment(new NoProxyEnvironment);

struct RecordedRequest {
	std::string method;
	std::string target;
	std::map<std::string, std::string> headers; // lower-case names
	std::string body;
};

/// Accepts connections on 127.0.0.1, records each request, and replies with a
/// fixed response, or never replies when `response` is empty.
class LoopbackServer {
public:
	explicit LoopbackServer(std::string response) : response_(std::move(response)) {
		initialize_sockets();
		listener_ = ::socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		address.sin_port = 0;
		if (listener_ == kInvalidSocket ||
			::bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
			::listen(listener_, 8) != 0) {
			return;
		}
		AddressLength length = sizeof(address);
		::getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length);
		port_ = ntohs(address.sin_port);
		thread_ = std::thread([this] { serve(); });
	}

	~LoopbackServer() {
		stopping_ = true;
		if (listener_ != kInvalidSocket) {
			::shutdown(listener_, kShutdownBoth);
			close_socket(listener_);
		}
		if (thread_.joinable()) {
			thread_.join();
		}
	}

	LoopbackServer(const LoopbackServer&) = delete;
	LoopbackServer& operator=(const LoopbackServer&) = delete;

	[[nodiscard]] std::string base_url() const {
		return "http://127.0.0.1:" + std::to_string(port_) + "/trade-api/v2";
	}

	[[nodiscard]] RecordedRequest last() const {
		const std::scoped_lock lock(mutex_);
		return requests_.empty() ? RecordedRequest{} : requests_.back();
	}

private:
	void serve() {
		while (!stopping_) {
			const Socket connection = ::accept(listener_, nullptr, nullptr);
			if (connection == kInvalidSocket) {
				return;
			}
			handle(connection);
			close_socket(connection);
		}
	}

	void handle(Socket connection) {
		std::string data;
		std::array<char, 4096> buffer{};
		std::size_t header_end = std::string::npos;
		while ((header_end = data.find("\r\n\r\n")) == std::string::npos) {
			const SocketIoResult received =
				::recv(connection, buffer.data(), static_cast<int>(buffer.size()), 0);
			if (received <= 0) {
				return;
			}
			data.append(buffer.data(), static_cast<std::size_t>(received));
		}

		RecordedRequest request;
		const std::string head = data.substr(0, header_end);
		const std::size_t line_end = head.find("\r\n");
		const std::string request_line = head.substr(0, line_end);
		request.method = request_line.substr(0, request_line.find(' '));
		const std::size_t target_start = request.method.size() + 1;
		request.target = request_line.substr(target_start, request_line.rfind(' ') - target_start);
		std::size_t position = line_end == std::string::npos ? head.size() : line_end + 2;
		while (position < head.size()) {
			const std::size_t next = std::min(head.find("\r\n", position), head.size());
			const std::string line = head.substr(position, next - position);
			const std::size_t colon = line.find(':');
			if (colon != std::string::npos) {
				std::string name = line.substr(0, colon);
				std::transform(name.begin(), name.end(), name.begin(),
							   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				request.headers[name] = line.substr(line.find_first_not_of(' ', colon + 1));
			}
			position = next + 2;
		}

		const std::size_t length = request.headers.contains("content-length")
									   ? std::stoul(request.headers["content-length"])
									   : 0;
		request.body = data.substr(header_end + 4);
		while (request.body.size() < length) {
			const SocketIoResult received =
				::recv(connection, buffer.data(), static_cast<int>(buffer.size()), 0);
			if (received <= 0) {
				break;
			}
			request.body.append(buffer.data(), static_cast<std::size_t>(received));
		}
		{
			const std::scoped_lock lock(mutex_);
			requests_.push_back(request);
		}

		if (response_.empty()) {
			while (!stopping_) {
				std::this_thread::sleep_for(10ms);
			}
			return;
		}
#ifdef SO_NOSIGPIPE
		const int no_sigpipe = 1;
		(void)::setsockopt(connection, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(no_sigpipe));
#endif
#ifdef MSG_NOSIGNAL
		constexpr int kSendFlags = MSG_NOSIGNAL;
#else
		constexpr int kSendFlags = 0;
#endif
		std::size_t sent = 0;
		while (sent < response_.size()) {
			const int chunk =
				static_cast<int>(std::min(response_.size() - sent, std::size_t{65536}));
			const SocketIoResult written =
				::send(connection, response_.data() + sent, chunk, kSendFlags);
			if (written <= 0) {
				return;
			}
			sent += static_cast<std::size_t>(written);
		}
	}

	std::string response_;
	Socket listener_{kInvalidSocket};
	std::uint16_t port_{0};
	std::atomic<bool> stopping_{false};
	std::thread thread_;
	mutable std::mutex mutex_;
	std::vector<RecordedRequest> requests_;
};

std::string http_response(int status, std::string_view body, std::string_view extra = {}) {
	return "HTTP/1.1 " + std::to_string(status) +
		   " Status\r\nContent-Length: " + std::to_string(body.size()) +
		   "\r\nConnection: close\r\n" + std::string(extra) + "\r\n" + std::string(body);
}

TEST(HttpClient, PreservesBodyAndEmptyHeaders) {
	LoopbackServer server(http_response(200, "hello", "X-Empty:   \r\n"));
	ncei::HttpClient client({.base_url = server.base_url()});
	const ncei::Result<ncei::HttpResponse> result = client.get("/weather");
	ASSERT_TRUE(result);
	EXPECT_EQ(result->body, "hello");
	EXPECT_EQ(result->headers.back().second, "");
}
TEST(HttpClient, BoundsDecompressedResponses) {
	LoopbackServer server(http_response(200, "12345"));
	ncei::HttpClient client({.base_url = server.base_url(), .max_response_bytes = 4});
	const ncei::Result<ncei::HttpResponse> result = client.get("/weather");
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().code, ncei::ErrorCode::InvalidRequest);
}
TEST(HttpClient, ZeroBodyLimitAllowsLargerResponses) {
	LoopbackServer server(http_response(200, "12345"));
	ncei::HttpClient client({.base_url = server.base_url(), .max_response_bytes = 0});
	ASSERT_TRUE(client.get("/weather"));
}
TEST(HttpClient, RejectsMovedFromClientsAndNonHttpProtocols) {
	ncei::HttpClient first({.base_url = ""});
	ncei::HttpClient second(std::move(first));
	EXPECT_FALSE(first.get("/weather"));
	EXPECT_TRUE(first.config().base_url.empty());
	EXPECT_FALSE(second.get("file:///etc/hosts"));
	EXPECT_FALSE(second.get(std::string("http://example.com/\0ignored", 27)));
}

TEST(CdoClient, EncodesOpaqueParametersAndPathIds) {
	LoopbackServer server(http_response(200, R"({"id":"ok","results":[]})"));
	ncei::CDOClient::Config config;
	config.token = "test-token";
	config.http.base_url = server.base_url();
	ncei::CDOClient client(config);
	ncei::CDOListParams params;
	params.station_id = "A&B/1";
	ASSERT_TRUE(client.get_datasets(params));
	EXPECT_EQ(server.last().target, "/trade-api/v2/datasets?stationid=A%26B%2F1");
	ASSERT_TRUE(client.get_dataset("A/B?x=1"));
	EXPECT_EQ(server.last().target, "/trade-api/v2/datasets/A%2FB%3Fx%3D1");
	EXPECT_EQ(server.last().headers.at("token"), "test-token");
}
TEST(CdoClient, EveryRetryConsumesDailyQuota) {
	LoopbackServer server(http_response(500, "{}"));
	ncei::CDOClient::Config config;
	config.token = "test-token";
	config.http.base_url = server.base_url();
	config.rate_limit.daily_limit = 2;
	config.retry.initial_delay = 0ms;
	config.retry.max_delay = 0ms;
	ncei::CDOClient client(config);
	const ncei::Result<ncei::CDOResponse<ncei::Dataset>> result = client.get_datasets({});
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().code, ncei::ErrorCode::QuotaExceeded);
	EXPECT_EQ(client.daily_requests_remaining(), 0);
}
TEST(HttpClient, RejectsHeaderInjection) {
	ncei::ClientConfig config;
	config.base_url = "http://127.0.0.1:1";
	config.extra_headers = {{"token", "abc\r\nInjected: yes"}};
	ncei::HttpClient client(config);
	const ncei::Result<ncei::HttpResponse> result = client.get("/data");
	ASSERT_FALSE(result);
	EXPECT_EQ(result.error().code, ncei::ErrorCode::InvalidRequest);
}
} // namespace
