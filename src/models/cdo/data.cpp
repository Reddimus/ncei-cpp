#include "ncei/models/cdo/data.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, DataRecord& d) {
	d.date = json_string(j, "date");
	d.datatype = json_string(j, "datatype");
	d.station = json_string(j, "station");
	d.attributes = json_string(j, "attributes");
	d.value = json_double(j, "value");
}

} // namespace ncei
