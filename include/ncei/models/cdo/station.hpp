#pragma once

#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ncei {

struct CDOStation {
	std::string id; // "GHCND:USW00013874"
	std::string name;
	double data_coverage{0.0};
	std::string min_date;
	std::string max_date;
	double latitude{0.0};
	double longitude{0.0};
	double elevation{0.0};
	std::string elevation_unit;
};

void from_json(const nlohmann::json& j, CDOStation& s);

} // namespace ncei
