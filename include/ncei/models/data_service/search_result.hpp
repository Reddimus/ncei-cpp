#pragma once
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <utility>
#include <vector>

namespace ncei {

struct DataSearchResult {
	std::string station_id;
	std::string station_name;
	double latitude{0.0};
	double longitude{0.0};
	double elevation{0.0};
	std::string min_date;
	std::string max_date;
	double data_coverage{0.0};
};

struct DatasetSearchResult {
	std::string uid;
	std::string name;
	std::string description;
	std::string start_date;
	std::string end_date;
	std::vector<std::string> data_types;
};

void from_json(const nlohmann::json& j, DataSearchResult& r);
void from_json(const nlohmann::json& j, DatasetSearchResult& r);

} // namespace ncei
