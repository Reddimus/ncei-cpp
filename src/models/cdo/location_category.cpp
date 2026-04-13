#include "ncei/models/cdo/location_category.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, LocationCategory& lc) {
	lc.id = json_string(j, "id");
	lc.name = json_string(j, "name");
}

} // namespace ncei
