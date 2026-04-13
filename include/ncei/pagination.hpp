#pragma once
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <vector>

namespace ncei {

struct ResultSetMetadata {
	std::int32_t offset{0};
	std::int32_t count{0};
	std::int32_t limit{25};
};

void from_json(const nlohmann::json& j, ResultSetMetadata& m);

template <typename T>
struct CDOResponse {
	ResultSetMetadata metadata;
	std::vector<T> results;

	[[nodiscard]] bool has_more() const noexcept {
		return metadata.offset + metadata.limit < metadata.count;
	}
	[[nodiscard]] std::int32_t next_offset() const noexcept {
		return metadata.offset + metadata.limit;
	}
};

} // namespace ncei
