#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ncei {

struct DataRecord {
	std::string date;
	std::string datatype; // "TMAX", "TMIN", etc.
	std::string station;
	std::string attributes;
	double value{0.0};
};

void from_json(const nlohmann::json& j, DataRecord& d);

} // namespace ncei
