#include "ncei/models/cdo/station.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, CDOStation& s) {
	s.id = json_string(j, "id");
	s.name = json_string(j, "name");
	s.data_coverage = json_double(j, "datacoverage");
	s.min_date = json_string(j, "mindate");
	s.max_date = json_string(j, "maxdate");
	s.latitude = json_double(j, "latitude");
	s.longitude = json_double(j, "longitude");
	s.elevation = json_double(j, "elevation");
	s.elevation_unit = json_string(j, "elevationUnit");
}

} // namespace ncei
