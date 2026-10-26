#include "ncei/rate_limit.hpp"

#include <algorithm>
#include <limits>

namespace ncei {
namespace {
RateLimiter::Config sanitized(RateLimiter::Config config) {
	config.max_tokens = std::max<std::uint16_t>(config.max_tokens, 1);
	config.initial_tokens = std::min(config.initial_tokens, config.max_tokens);
	if (config.refill_interval <= std::chrono::milliseconds::zero()) {
		config.refill_interval = std::chrono::milliseconds{200};
	}
	return config;
}
} // namespace

RateLimiter::RateLimiter(Config config)
	: config_(sanitized(config)), tokens_(config_.initial_tokens),
	  last_refill_(std::chrono::steady_clock::now()),
	  day_start_(std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())) {}

void RateLimiter::check_daily_reset() noexcept {
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::system_clock::time_point current_day = std::chrono::floor<std::chrono::days>(now);
	if (current_day > day_start_) {
		daily_requests_used_ = 0;
		day_start_ = current_day;
	}
}

void RateLimiter::refill() noexcept {
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (tokens_ >= config_.max_tokens) {
		last_refill_ = now;
		return;
	}
	const std::int64_t earned =
		std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_).count() /
		config_.refill_interval.count();
	if (earned <= 0) {
		return;
	}
	const std::int64_t total = static_cast<std::int64_t>(tokens_) + earned;
	if (total >= config_.max_tokens) {
		tokens_ = config_.max_tokens;
		last_refill_ = now;
	} else {
		tokens_ = static_cast<std::uint16_t>(total);
		last_refill_ += earned * config_.refill_interval;
	}
}

bool RateLimiter::try_acquire() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	check_daily_reset();

	// Check daily limit (0 = no daily limit)
	if (config_.daily_limit > 0 && daily_requests_used_ >= config_.daily_limit) {
		return false;
	}

	refill();
	if (tokens_ > 0) {
		--tokens_;
		if (daily_requests_used_ < std::numeric_limits<std::int32_t>::max()) {
			++daily_requests_used_;
		}
		return true;
	}
	return false;
}

bool RateLimiter::acquire() {
	if (config_.max_wait) {
		return acquire_for(*config_.max_wait);
	}

	// No max_wait: block indefinitely
	while (true) {
		if (try_acquire()) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

bool RateLimiter::acquire_for(std::chrono::milliseconds max_wait) {
	using Clock = std::chrono::steady_clock;
	const Clock::time_point now = Clock::now();
	const std::chrono::milliseconds remaining =
		std::chrono::duration_cast<std::chrono::milliseconds>(Clock::time_point::max() - now);
	const Clock::time_point deadline = max_wait <= std::chrono::milliseconds::zero() ? now
									   : max_wait >= remaining ? Clock::time_point::max()
															   : now + max_wait;
	while (true) {
		if (try_acquire()) {
			return true;
		}
		const Clock::time_point current = Clock::now();
		if (current >= deadline) {
			return false;
		}
		std::this_thread::sleep_for(
			std::min(deadline - current,
					 std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds{10})));
	}
}

std::uint16_t RateLimiter::available_tokens() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	const std::int64_t earned = std::chrono::duration_cast<std::chrono::milliseconds>(
									std::chrono::steady_clock::now() - last_refill_)
									.count() /
								config_.refill_interval.count();
	return static_cast<std::uint16_t>(std::min<std::int64_t>(static_cast<std::int64_t>(tokens_) +
																 std::max<std::int64_t>(earned, 0),
															 config_.max_tokens));
}

std::int32_t RateLimiter::daily_requests_remaining() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	if (config_.daily_limit <= 0) {
		// daily_limit <= 0 means "no daily cap" (see rate_limit.hpp). Signal
		// unlimited headroom — NOT 0, which callers (CDOClient::do_get) treat
		// as quota-exhausted and would reject every request when the cap is
		// intentionally disabled.
		return std::numeric_limits<std::int32_t>::max();
	}
	if (std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now()) > day_start_) {
		return config_.daily_limit;
	}
	return std::max(config_.daily_limit - daily_requests_used_, 0);
}

void RateLimiter::reset() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	tokens_ = config_.initial_tokens;
	last_refill_ = std::chrono::steady_clock::now();
	daily_requests_used_ = 0;
	day_start_ = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
}

const RateLimiter::Config& RateLimiter::config() const noexcept {
	return config_;
}

// ScopedRateLimit

ScopedRateLimit::ScopedRateLimit(RateLimiter& limiter) : acquired_(limiter.acquire()) {}

bool ScopedRateLimit::acquired() const noexcept {
	return acquired_;
}

ScopedRateLimit::operator bool() const noexcept {
	return acquired_;
}

} // namespace ncei
