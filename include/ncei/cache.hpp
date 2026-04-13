#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace ncei {

/// Thread-safe LRU cache with time-to-live expiration
template <typename Key, typename Value>
class LruCache {
public:
	struct Config {
		std::size_t max_entries{1000};
		std::chrono::seconds ttl{3600};
	};

	explicit LruCache(Config config = {}) : config_(std::move(config)) {}

	/// Get a value from the cache, returns nullopt on miss or expiry
	[[nodiscard]] std::optional<Value> get(const Key& key) {
		std::lock_guard lock(mutex_);
		auto it = map_.find(key);
		if (it == map_.end()) {
			return std::nullopt;
		}

		CacheEntry& entry = it->second;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (now - entry.inserted_at > config_.ttl) {
			// Expired: remove from LRU list and map
			order_.erase(entry.list_it);
			map_.erase(it);
			return std::nullopt;
		}

		// Move to front of LRU list (most recently used)
		order_.splice(order_.begin(), order_, entry.list_it);
		return entry.value;
	}

	/// Insert or update a value in the cache
	void put(const Key& key, Value value) {
		std::lock_guard lock(mutex_);
		auto it = map_.find(key);
		if (it != map_.end()) {
			// Update existing entry
			it->second.value = std::move(value);
			it->second.inserted_at = std::chrono::steady_clock::now();
			order_.splice(order_.begin(), order_, it->second.list_it);
			return;
		}

		// Evict if at capacity
		while (map_.size() >= config_.max_entries && !order_.empty()) {
			Key& lru_key = order_.back();
			map_.erase(lru_key);
			order_.pop_back();
		}

		// Insert new entry
		order_.push_front(key);
		CacheEntry entry;
		entry.value = std::move(value);
		entry.inserted_at = std::chrono::steady_clock::now();
		entry.list_it = order_.begin();
		map_.emplace(key, std::move(entry));
	}

	/// Remove a specific key from the cache
	void invalidate(const Key& key) {
		std::lock_guard lock(mutex_);
		auto it = map_.find(key);
		if (it != map_.end()) {
			order_.erase(it->second.list_it);
			map_.erase(it);
		}
	}

	/// Remove all entries from the cache
	void clear() {
		std::lock_guard lock(mutex_);
		map_.clear();
		order_.clear();
	}

	/// Return the number of entries currently in the cache
	[[nodiscard]] std::size_t size() const {
		std::lock_guard lock(mutex_);
		return map_.size();
	}

	/// Get the cache configuration
	[[nodiscard]] const Config& config() const noexcept { return config_; }

private:
	struct CacheEntry {
		Value value;
		std::chrono::steady_clock::time_point inserted_at;
		typename std::list<Key>::iterator list_it;
	};

	Config config_;
	mutable std::mutex mutex_;
	std::list<Key> order_; // front = most recent, back = least recent
	std::unordered_map<Key, CacheEntry> map_;
};

/// Key type for coordinate-based cache lookups
struct CoordinateKey {
	double latitude{0.0};
	double longitude{0.0};

	[[nodiscard]] bool operator==(const CoordinateKey&) const = default;
};

} // namespace ncei

/// std::hash specialization for CoordinateKey
template <>
struct std::hash<ncei::CoordinateKey> {
	std::size_t operator()(const ncei::CoordinateKey& k) const noexcept {
		std::size_t h1 = std::hash<double>{}(k.latitude);
		std::size_t h2 = std::hash<double>{}(k.longitude);
		return h1 ^ (h2 << 1);
	}
};
