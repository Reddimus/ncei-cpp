#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>

namespace ncei {

/// Error codes for NCEI SDK operations
enum class ErrorCode {
	Ok = 0,
	NetworkError,
	RateLimited,
	QuotaExceeded,
	ServerError,
	NotFound,
	InvalidRequest,
	ParseError,
	CacheError,
	Unknown
};

/// Convert ErrorCode to string
[[nodiscard]] constexpr std::string_view to_string(ErrorCode code) noexcept {
	switch (code) {
		case ErrorCode::Ok:
			return "Ok";
		case ErrorCode::NetworkError:
			return "NetworkError";
		case ErrorCode::RateLimited:
			return "RateLimited";
		case ErrorCode::QuotaExceeded:
			return "QuotaExceeded";
		case ErrorCode::ServerError:
			return "ServerError";
		case ErrorCode::NotFound:
			return "NotFound";
		case ErrorCode::InvalidRequest:
			return "InvalidRequest";
		case ErrorCode::ParseError:
			return "ParseError";
		case ErrorCode::CacheError:
			return "CacheError";
		case ErrorCode::Unknown:
			return "Unknown";
	}
	return "Unknown";
}

/// Error information returned by SDK operations.
/// Handles CDO, Data Service, and RFC 7807 error formats.
struct Error {
	ErrorCode code;
	std::string message;
	int http_status{0};
	std::string correlation_id;
	std::string detail;

	[[nodiscard]] constexpr bool is_ok() const noexcept { return code == ErrorCode::Ok; }

	[[nodiscard]] static Error ok() { return {ErrorCode::Ok, ""}; }

	[[nodiscard]] static Error network(std::string msg) {
		return {ErrorCode::NetworkError, std::move(msg)};
	}

	[[nodiscard]] static Error parse(std::string msg) {
		return {ErrorCode::ParseError, std::move(msg)};
	}

	[[nodiscard]] static Error not_found(std::string msg) {
		return {ErrorCode::NotFound, std::move(msg)};
	}

	[[nodiscard]] static Error rate_limited(std::string msg) {
		return {ErrorCode::RateLimited, std::move(msg)};
	}

	[[nodiscard]] static Error quota_exceeded(std::string msg) {
		return {ErrorCode::QuotaExceeded, std::move(msg)};
	}

	[[nodiscard]] static Error server(std::string msg) {
		return {ErrorCode::ServerError, std::move(msg)};
	}

	[[nodiscard]] static Error invalid_request(std::string msg) {
		return {ErrorCode::InvalidRequest, std::move(msg)};
	}

	/// Create an Error from an HTTP response status code and body
	[[nodiscard]] static Error from_response(int status, const std::string& body,
											 const std::string& correlation_id = {});
};

/// Result type for SDK operations
template <typename T>
using Result = std::expected<T, Error>;

} // namespace ncei
