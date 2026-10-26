#include "ncei/http_client.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <limits>
#include <memory>

namespace ncei {
namespace {

bool is_absolute_url(std::string_view value) {
	const std::size_t end = value.find("://");
	if (end == std::string_view::npos || end == 0) {
		return false;
	}
	const char first = value.front();
	if (!((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z'))) {
		return false;
	}
	return value.substr(0, end).find_first_not_of(
			   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+.-") ==
		   std::string_view::npos;
}

class CurlRuntime {
public:
	CurlRuntime() : status_(curl_global_init(CURL_GLOBAL_DEFAULT)) {}
	~CurlRuntime() {
		if (status_ == CURLE_OK) {
			curl_global_cleanup();
		}
	}
	CurlRuntime(const CurlRuntime&) = delete;
	CurlRuntime& operator=(const CurlRuntime&) = delete;
	[[nodiscard]] CURLcode status() const noexcept { return status_; }

private:
	CURLcode status_;
};

std::shared_ptr<const CurlRuntime> runtime() {
	static const std::shared_ptr<const CurlRuntime> value = std::make_shared<const CurlRuntime>();
	return value;
}

struct ResponseSink {
	HttpResponse response{};
	std::size_t limit;
	bool overflowed{false};
};

// Libcurl's C stack must never unwind through a C++ allocation failure.
std::size_t write_callback(char* data, std::size_t size, std::size_t count,
						   void* userdata) noexcept {
	ResponseSink& sink = *static_cast<ResponseSink*>(userdata);
	if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
		return 0;
	}
	const std::size_t bytes = size * count;
	if (sink.limit > 0 && bytes > sink.limit - sink.response.body.size()) {
		sink.overflowed = true;
		return 0;
	}
	try {
		sink.response.body.append(data, bytes);
	} catch (...) {
		return 0;
	}
	return bytes;
}

std::size_t header_callback(char* data, std::size_t size, std::size_t count,
							void* userdata) noexcept {
	ResponseSink& sink = *static_cast<ResponseSink*>(userdata);
	if (size != 0 && count > std::numeric_limits<std::size_t>::max() / size) {
		return 0;
	}
	const std::size_t bytes = size * count;
	try {
		const std::string_view line(data, bytes);
		if (line.starts_with("HTTP/")) {
			sink.response.headers.clear();
		}
		const std::size_t colon = line.find(':');
		if (colon != std::string_view::npos) {
			const std::string_view raw = line.substr(colon + 1);
			const std::size_t start = raw.find_first_not_of(" \t\r\n");
			const std::size_t end = raw.find_last_not_of(" \t\r\n");
			const std::string_view value = start == std::string_view::npos
											   ? std::string_view{}
											   : raw.substr(start, end - start + 1);
			sink.response.headers.emplace_back(line.substr(0, colon), value);
		}
	} catch (...) {
		return 0;
	}
	return bytes;
}

bool invalid_header(std::string_view value) {
	return value.find_first_of(std::string_view{"\r\n\0", 3}) != std::string_view::npos;
}
} // namespace

struct HttpClient::Impl {
	// Kept alive until after this instance's easy handle has been closed.
	std::shared_ptr<const CurlRuntime> global{runtime()};
	ClientConfig config;
	CURL* curl{global->status() == CURLE_OK ? curl_easy_init() : nullptr};
	explicit Impl(ClientConfig value) : config(std::move(value)) {}
	~Impl() {
		if (curl != nullptr) {
			curl_easy_cleanup(curl);
		}
	}
	Impl(const Impl&) = delete;
	Impl& operator=(const Impl&) = delete;
};

HttpClient::HttpClient(ClientConfig config) : impl_(std::make_unique<Impl>(std::move(config))) {}
HttpClient::~HttpClient() = default;
HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

Result<HttpResponse> HttpClient::get(std::string_view path) const {
	if (!impl_ || impl_->curl == nullptr) {
		return std::unexpected(Error::network("CURL not initialized"));
	}
	const ClientConfig& config = impl_->config;
	const std::string url =
		!is_absolute_url(path) ? config.base_url + std::string(path) : std::string(path);
	if (url.find('\0') != std::string::npos || invalid_header(config.user_agent) ||
		invalid_header(config.accept) || config.timeout < std::chrono::seconds::zero()) {
		return std::unexpected(Error::invalid_request("Invalid URL, header, or timeout"));
	}
	using Headers = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
	const std::string accept = "Accept: " + config.accept;
	Headers headers(curl_slist_append(nullptr, accept.c_str()), &curl_slist_free_all);
	if (!headers) {
		return std::unexpected(Error::network("Could not allocate HTTP headers"));
	}

	for (const auto& [key, value] : config.extra_headers) {
		if (key.empty() || key.find(':') != std::string::npos || invalid_header(key) ||
			invalid_header(value)) {
			return std::unexpected(Error::invalid_request("Invalid request header"));
		}
		curl_slist* appended = curl_slist_append(headers.get(), (key + ": " + value).c_str());
		if (!appended) {
			return std::unexpected(Error::network("Could not allocate request headers"));
		}
		(void)headers.release();
		headers.reset(appended);
	}
	ResponseSink sink{{}, config.max_response_bytes};
	CURL* curl = impl_->curl;
	curl_easy_reset(curl);
	const long timeout = static_cast<long>(
		std::min(config.timeout.count(),
				 static_cast<std::chrono::seconds::rep>(std::numeric_limits<long>::max())));
	const long verify = config.verify_ssl ? 1L : 0L;
	const bool configured =
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str()) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http,https") == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "http,https") == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_callback) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &header_callback) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &sink) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers.get()) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_USERAGENT, config.user_agent.c_str()) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "") == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config.extra_headers.empty() ? 1L : 0L) ==
			CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verify) == CURLE_OK &&
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verify * 2L) == CURLE_OK;
	if (!configured) {
		return std::unexpected(Error::network("libcurl rejected a required option"));
	}
	const CURLcode result = curl_easy_perform(curl);
	if (sink.overflowed) {
		return std::unexpected(
			Error::invalid_request("Response exceeded ClientConfig::max_response_bytes"));
	}
	if (result != CURLE_OK) {
		return std::unexpected(Error::network(curl_easy_strerror(result)));
	}
	long status = 0;
	if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK) {
		return std::unexpected(Error::network("libcurl did not return HTTP status"));
	}
	sink.response.status_code = static_cast<std::int16_t>(status);
	return std::move(sink.response);
}

const ClientConfig& HttpClient::config() const noexcept {
	if (impl_) {
		return impl_->config;
	}
	static const ClientConfig moved_from{.base_url = {}, .user_agent = {}, .accept = {}};
	return moved_from;
}
} // namespace ncei
