#pragma once

#include <chrono>
#include <cmath>
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
		if (std::chrono::duration<long double>(now - entry.inserted_at) >
			std::chrono::duration<long double>(config_.ttl)) {
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
		if (config_.max_entries == 0 || config_.ttl <= std::chrono::seconds::zero()) {
			return;
		}
		std::lock_guard lock(mutex_);
		auto it = map_.find(key);
		if (it != map_.end()) {
			// Update existing entry
			it->second.value = std::move(value);
			it->second.inserted_at = std::chrono::steady_clock::now();
			order_.splice(order_.begin(), order_, it->second.list_it);
			return;
		}

		// Roll back the list entry if allocating or copying the map entry fails.
		order_.push_front(key);
		try {
			map_.emplace(key, CacheEntry{std::move(value), std::chrono::steady_clock::now(),
										 order_.begin()});
		} catch (...) {
			order_.pop_front();
			throw;
		}
		while (map_.size() > config_.max_entries) {
			map_.erase(order_.back());
			order_.pop_back();
		}
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

	[[nodiscard]] bool operator==(const CoordinateKey& other) const noexcept {
		return (latitude == other.latitude ||
				(std::isnan(latitude) && std::isnan(other.latitude))) &&
			   (longitude == other.longitude ||
				(std::isnan(longitude) && std::isnan(other.longitude)));
	}
};

} // namespace ncei

/// std::hash specialization for CoordinateKey
template <>
struct std::hash<ncei::CoordinateKey> {
	std::size_t operator()(const ncei::CoordinateKey& k) const noexcept {
		std::size_t h1 = std::hash<double>{}(std::isnan(k.latitude) ? 0.0 : k.latitude);
		std::size_t h2 = std::hash<double>{}(std::isnan(k.longitude) ? 0.0 : k.longitude);
		return h1 ^ (h2 << 1);
	}
};
