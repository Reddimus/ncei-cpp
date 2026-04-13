#pragma once

#include "ncei/error.hpp"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

namespace ncei {

/// Token bucket rate limiter with daily quota tracking
///
/// Implements a token bucket algorithm to limit request rates.
/// Also tracks daily request counts for CDO API quota enforcement.
/// Thread-safe for concurrent access.
class RateLimiter {
public:
	/// Configuration for rate limiting
	struct Config {
		std::uint16_t max_tokens = 5;					///< Maximum tokens (5/sec for CDO)
		std::chrono::milliseconds refill_interval{200}; ///< Time to add one token (200ms = 5/sec)
		std::uint16_t initial_tokens = 5;				///< Starting tokens
		std::optional<std::chrono::milliseconds> max_wait; // Max time to wait
		std::int32_t daily_limit{10000};				   ///< Daily request limit (0 = no limit)
	};

	explicit RateLimiter(Config config);

	/// Try to acquire a token, returns true if successful
	[[nodiscard]] bool try_acquire() noexcept;

	/// Acquire a token, blocking if necessary
	/// Returns false if max_wait exceeded
	[[nodiscard]] bool acquire();

	/// Acquire a token, blocking up to max_wait
	[[nodiscard]] bool acquire_for(std::chrono::milliseconds max_wait);

	/// Get current number of available tokens
	[[nodiscard]] std::uint16_t available_tokens() const noexcept;

	/// Get number of daily requests remaining
	[[nodiscard]] std::int32_t daily_requests_remaining() const noexcept;

	/// Reset the rate limiter to initial state
	void reset() noexcept;

	/// Get the configuration
	[[nodiscard]] const Config& config() const noexcept;

private:
	void refill() noexcept;
	void check_daily_reset() noexcept;

	Config config_;
	mutable std::mutex mutex_;
	std::uint16_t tokens_;
	std::chrono::steady_clock::time_point last_refill_;
	std::int32_t daily_requests_used_{0};
	std::chrono::system_clock::time_point day_start_;
};

/// Scoped rate limit acquisition
///
/// RAII wrapper that acquires a rate limit token on construction.
class ScopedRateLimit {
public:
	explicit ScopedRateLimit(RateLimiter& limiter);

	/// Check if acquisition was successful
	[[nodiscard]] bool acquired() const noexcept;

	/// Implicit conversion to bool for easy checking
	explicit operator bool() const noexcept;

private:
	bool acquired_;
};

} // namespace ncei
