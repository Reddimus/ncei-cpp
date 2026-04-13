#include "ncei/models/cdo/dataset.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, Dataset& d) {
	d.id = json_string(j, "id");
	d.name = json_string(j, "name");
	d.data_coverage = json_double(j, "datacoverage");
	d.min_date = json_string(j, "mindate");
	d.max_date = json_string(j, "maxdate");
}

} // namespace ncei
