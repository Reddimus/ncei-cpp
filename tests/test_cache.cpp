#include "ncei/cache.hpp"

#include <gtest/gtest.h>
#include <thread>

namespace ncei {
namespace {

TEST(LruCacheTest, PutAndGet) {
	LruCache<std::string, int> cache;
	cache.put("a", 1);
	std::optional<int> val = cache.get("a");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 1);
}

TEST(LruCacheTest, MissReturnsNullopt) {
	LruCache<std::string, int> cache;
	std::optional<int> val = cache.get("nonexistent");
	EXPECT_FALSE(val.has_value());
}

TEST(LruCacheTest, UpdateExisting) {
	LruCache<std::string, int> cache;
	cache.put("a", 1);
	cache.put("a", 2);
	std::optional<int> val = cache.get("a");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, 2);
	EXPECT_EQ(cache.size(), 1);
}

TEST(LruCacheTest, EvictsLeastRecentlyUsed) {
	LruCache<std::string, int> cache({.max_entries = 2, .ttl = std::chrono::seconds(3600)});
	cache.put("a", 1);
	cache.put("b", 2);
	cache.put("c", 3); // Should evict "a"
	EXPECT_FALSE(cache.get("a").has_value());
	EXPECT_TRUE(cache.get("b").has_value());
	EXPECT_TRUE(cache.get("c").has_value());
}

TEST(LruCacheTest, GetRefreshesOrder) {
	LruCache<std::string, int> cache({.max_entries = 2, .ttl = std::chrono::seconds(3600)});
	cache.put("a", 1);
	cache.put("b", 2);
	(void)cache.get("a"); // Refresh "a" to most recent
	cache.put("c", 3);	  // Should evict "b" (least recent), not "a"
	EXPECT_TRUE(cache.get("a").has_value());
	EXPECT_FALSE(cache.get("b").has_value());
	EXPECT_TRUE(cache.get("c").has_value());
}

TEST(LruCacheTest, TTLExpiry) {
	LruCache<std::string, int> cache(
		{.max_entries = 100, .ttl = std::chrono::seconds(0)}); // Immediate expiry
	cache.put("a", 1);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	std::optional<int> val = cache.get("a");
	EXPECT_FALSE(val.has_value());
}

TEST(LruCacheTest, Invalidate) {
	LruCache<std::string, int> cache;
	cache.put("a", 1);
	cache.invalidate("a");
	EXPECT_FALSE(cache.get("a").has_value());
	EXPECT_EQ(cache.size(), 0);
}

TEST(LruCacheTest, InvalidateNonexistent) {
	LruCache<std::string, int> cache;
	cache.invalidate("nonexistent"); // Should not throw
	EXPECT_EQ(cache.size(), 0);
}

TEST(LruCacheTest, Clear) {
	LruCache<std::string, int> cache;
	cache.put("a", 1);
	cache.put("b", 2);
	cache.clear();
	EXPECT_EQ(cache.size(), 0);
	EXPECT_FALSE(cache.get("a").has_value());
}

TEST(LruCacheTest, Size) {
	LruCache<std::string, int> cache;
	EXPECT_EQ(cache.size(), 0);
	cache.put("a", 1);
	EXPECT_EQ(cache.size(), 1);
	cache.put("b", 2);
	EXPECT_EQ(cache.size(), 2);
}

TEST(CoordinateKeyTest, Equality) {
	CoordinateKey a{39.7456, -97.0892};
	CoordinateKey b{39.7456, -97.0892};
	EXPECT_EQ(a, b);
}

TEST(CoordinateKeyTest, Inequality) {
	CoordinateKey a{39.7456, -97.0892};
	CoordinateKey b{40.0, -97.0892};
	EXPECT_NE(a, b);
}

TEST(CoordinateKeyTest, HashWorks) {
	std::hash<CoordinateKey> hasher;
	CoordinateKey a{39.7456, -97.0892};
	CoordinateKey b{39.7456, -97.0892};
	EXPECT_EQ(hasher(a), hasher(b));
}

TEST(CoordinateKeyTest, CacheWithCoordinateKey) {
	LruCache<CoordinateKey, std::string> cache;
	CoordinateKey key{39.7456, -97.0892};
	cache.put(key, "Topeka");
	std::optional<std::string> val = cache.get(key);
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, "Topeka");
}

} // namespace
} // namespace ncei
