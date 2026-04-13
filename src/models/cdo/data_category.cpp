#include "ncei/models/cdo/data_category.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, DataCategory& d) {
	d.id = json_string(j, "id");
	d.name = json_string(j, "name");
}

} // namespace ncei
