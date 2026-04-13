#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ncei {

struct LocationCategory {
	std::string id; // "CITY", "ST", "CNTRY", etc.
	std::string name;
};

void from_json(const nlohmann::json& j, LocationCategory& lc);

} // namespace ncei
