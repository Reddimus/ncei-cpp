#include "ncei/models/data_service/dataset_metadata.hpp"

#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

void from_json(const nlohmann::json& j, DatasetField& f) {
	f.id = json_string(j, "id");
	f.name = json_string(j, "name");
	f.description = json_string(j, "description");
	f.data_type = json_string(j, "dataType");
}

void from_json(const nlohmann::json& j, DatasetMetadata& m) {
	m.id = json_string(j, "id");
	m.name = json_string(j, "name");
	m.description = json_string(j, "description");

	m.fields.clear();
	if (j.contains("fields") && j["fields"].is_array()) {
		for (const nlohmann::json& item : j["fields"]) {
			DatasetField f;
			from_json(item, f);
			m.fields.push_back(std::move(f));
		}
	}
}

} // namespace ncei
