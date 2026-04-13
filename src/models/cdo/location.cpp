#include "ncei/models/cdo/location.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, Location& l) {
	l.id = json_string(j, "id");
	l.name = json_string(j, "name");
	l.data_coverage = json_double(j, "datacoverage");
	l.min_date = json_string(j, "mindate");
	l.max_date = json_string(j, "maxdate");
}

} // namespace ncei
