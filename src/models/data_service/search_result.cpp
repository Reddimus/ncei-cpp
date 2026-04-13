#include "ncei/models/data_service/search_result.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, DataSearchResult& r) {
	r.station_id = json_string(j, "id");
	r.station_name = json_string(j, "name");
	r.latitude = json_double(j, "latitude");
	r.longitude = json_double(j, "longitude");
	r.elevation = json_double(j, "elevation");
	r.min_date = json_string(j, "mindate");
	r.max_date = json_string(j, "maxdate");
	r.data_coverage = json_double(j, "datacoverage");
}

void from_json(const nlohmann::json& j, DatasetSearchResult& r) {
	r.uid = json_string(j, "uid");
	r.name = json_string(j, "name");
	r.description = json_string(j, "description");
	r.start_date = json_string(j, "startDate");
	r.end_date = json_string(j, "endDate");

	r.data_types.clear();
	if (j.contains("dataTypes") && j["dataTypes"].is_array()) {
		for (const nlohmann::json& dt : j["dataTypes"]) {
			if (dt.is_string()) {
				r.data_types.push_back(dt.get<std::string>());
			}
		}
	}
}

} // namespace ncei
