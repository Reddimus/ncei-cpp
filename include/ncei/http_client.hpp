#pragma once

#include "ncei/error.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ncei {

/// HTTP response
/// Uses contiguous vector storage for headers instead of unordered_map
/// for better cache locality (typically <10 headers in a response).
struct HttpResponse {
	std::int16_t status_code; // HTTP status codes fit in int16 (100-599)
	std::string body;
	std::vector<std::pair<std::string, std::string>> headers;
};

/// HTTP client configuration
struct ClientConfig {
	std::string base_url;
	std::string user_agent{"ncei-cpp/0.1.0"};
	std::string accept{"application/json"};
	std::chrono::seconds timeout{30};
	bool verify_ssl{true};
	std::vector<std::pair<std::string, std::string>> extra_headers;
};

/// HTTP client for NCEI APIs
///
/// GET-only client since the NCEI APIs are read-only.
///
/// @note Thread Safety: This class is NOT thread-safe. The underlying CURL
/// handle is shared across all requests. If you need concurrent API access,
/// create one HttpClient instance per thread or protect access with a mutex.
class HttpClient {
public:
	/// Create a client with the given configuration
	explicit HttpClient(ClientConfig config);
	~HttpClient();

	HttpClient(HttpClient&&) noexcept;
	HttpClient& operator=(HttpClient&&) noexcept;

	// Non-copyable
	HttpClient(const HttpClient&) = delete;
	HttpClient& operator=(const HttpClient&) = delete;

	/// Perform a GET request
	[[nodiscard]] Result<HttpResponse> get(std::string_view path) const;

	/// Get the client configuration
	[[nodiscard]] const ClientConfig& config() const noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ncei
