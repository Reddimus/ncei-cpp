#include "ncei/pagination.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, ResultSetMetadata& m) {
	if (j.contains("metadata") && j["metadata"].is_object()) {
		const nlohmann::json& meta = j["metadata"];
		if (meta.contains("resultset") && meta["resultset"].is_object()) {
			const nlohmann::json& rs = meta["resultset"];
			m.offset = json_int(rs, "offset", 0);
			m.count = json_int(rs, "count", 0);
			m.limit = json_int(rs, "limit", 25);
		}
	}
}

} // namespace ncei
