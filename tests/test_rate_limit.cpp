#include "ncei/rate_limit.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(RateLimiterTest, InitialTokens) {
	RateLimiter limiter({.max_tokens = 5, .initial_tokens = 5});
	EXPECT_EQ(limiter.available_tokens(), 5);
}

TEST(RateLimiterTest, AcquireDecrements) {
	RateLimiter limiter({.max_tokens = 5, .initial_tokens = 5});
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_EQ(limiter.available_tokens(), 4);
}

TEST(RateLimiterTest, AcquireAllTokens) {
	RateLimiter limiter({.max_tokens = 3, .initial_tokens = 3});
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_FALSE(limiter.try_acquire());
	EXPECT_EQ(limiter.available_tokens(), 0);
}

TEST(RateLimiterTest, Reset) {
	RateLimiter limiter({.max_tokens = 5, .initial_tokens = 5});
	(void)limiter.try_acquire();
	(void)limiter.try_acquire();
	limiter.reset();
	EXPECT_EQ(limiter.available_tokens(), 5);
}

TEST(RateLimiterTest, RefillAfterInterval) {
	RateLimiter limiter({
		.max_tokens = 2,
		.refill_interval = std::chrono::milliseconds(50),
		.initial_tokens = 0,
	});

	EXPECT_FALSE(limiter.try_acquire());
	std::this_thread::sleep_for(std::chrono::milliseconds(60));
	EXPECT_TRUE(limiter.try_acquire());
}

TEST(RateLimiterTest, DoesNotExceedMax) {
	RateLimiter limiter({
		.max_tokens = 2,
		.refill_interval = std::chrono::milliseconds(10),
		.initial_tokens = 2,
	});

	// Wait for potential refills, then check we didn't exceed max
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	EXPECT_LE(limiter.available_tokens(), 2);
}

TEST(RateLimiterTest, AcquireForTimesOut) {
	RateLimiter limiter({
		.max_tokens = 1,
		.refill_interval = std::chrono::milliseconds(5000), // Very slow refill
		.initial_tokens = 0,
	});

	EXPECT_FALSE(limiter.acquire_for(std::chrono::milliseconds(50)));
}

TEST(RateLimiterTest, AcquireForSucceeds) {
	RateLimiter limiter({
		.max_tokens = 1,
		.refill_interval = std::chrono::milliseconds(30),
		.initial_tokens = 0,
	});

	EXPECT_TRUE(limiter.acquire_for(std::chrono::milliseconds(200)));
}

TEST(RateLimiterTest, Config) {
	RateLimiter::Config config{.max_tokens = 10, .initial_tokens = 5};
	RateLimiter limiter(config);
	EXPECT_EQ(limiter.config().max_tokens, 10);
	EXPECT_EQ(limiter.config().initial_tokens, 5);
}

TEST(RateLimiterTest, DailyCounterDecrements) {
	RateLimiter limiter({
		.max_tokens = 5,
		.initial_tokens = 5,
		.daily_limit = 100,
	});

	EXPECT_EQ(limiter.daily_requests_remaining(), 100);
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_EQ(limiter.daily_requests_remaining(), 99);
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_EQ(limiter.daily_requests_remaining(), 98);
}

TEST(RateLimiterTest, DailyLimitExceeded) {
	RateLimiter limiter({
		.max_tokens = 5,
		.refill_interval = std::chrono::milliseconds(1),
		.initial_tokens = 5,
		.daily_limit = 3,
	});

	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
	// Daily limit reached, even though tokens may have refilled
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	EXPECT_FALSE(limiter.try_acquire());
	EXPECT_EQ(limiter.daily_requests_remaining(), 0);
}

TEST(RateLimiterTest, NoDailyLimitWhenZero) {
	RateLimiter limiter({
		.max_tokens = 5,
		.refill_interval = std::chrono::milliseconds(1),
		.initial_tokens = 5,
		.daily_limit = 0,
	});

	// Should not be limited by daily counter when daily_limit is 0
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
	EXPECT_TRUE(limiter.try_acquire());
}

TEST(RateLimiterTest, DailyCounterResetsOnReset) {
	RateLimiter limiter({
		.max_tokens = 5,
		.initial_tokens = 5,
		.daily_limit = 100,
	});

	(void)limiter.try_acquire();
	(void)limiter.try_acquire();
	EXPECT_EQ(limiter.daily_requests_remaining(), 98);
	limiter.reset();
	EXPECT_EQ(limiter.daily_requests_remaining(), 100);
}

TEST(ScopedRateLimitTest, AcquiresToken) {
	RateLimiter limiter({.max_tokens = 1, .initial_tokens = 1});
	{
		ScopedRateLimit scoped(limiter);
		EXPECT_TRUE(scoped.acquired());
		EXPECT_TRUE(static_cast<bool>(scoped));
	}
	EXPECT_EQ(limiter.available_tokens(), 0);
}

} // namespace
} // namespace ncei
