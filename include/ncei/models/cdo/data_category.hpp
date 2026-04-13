#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ncei {

struct DataCategory {
	std::string id; // "TEMP", "PRECIP", etc.
	std::string name;
};

void from_json(const nlohmann::json& j, DataCategory& d);

} // namespace ncei
