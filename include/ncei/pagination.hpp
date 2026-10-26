#pragma once
#include "ncei/error.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace ncei {

struct ResultSetMetadata {
	std::int32_t offset{0};
	std::int32_t count{0};
	std::int32_t limit{25};
};

template <typename T>
struct CDOResponse {
	ResultSetMetadata metadata;
	std::vector<T> results;

	[[nodiscard]] bool has_more() const noexcept {
		const std::int64_t next =
			static_cast<std::int64_t>(std::max(metadata.offset, 1)) + metadata.limit;
		return metadata.offset >= 0 && metadata.limit > 0 && next <= metadata.count;
	}
	[[nodiscard]] std::int32_t next_offset() const noexcept {
		return static_cast<std::int32_t>(std::clamp<std::int64_t>(
			static_cast<std::int64_t>(std::max(metadata.offset, 1)) + std::max(metadata.limit, 0),
			1, std::numeric_limits<std::int32_t>::max()));
	}
};

} // namespace ncei
