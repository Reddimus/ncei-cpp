#pragma once
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace ncei {

struct DatasetField {
	std::string id;
	std::string name;
	std::string description;
	std::string data_type; // "string", "number", etc.
};

struct DatasetMetadata {
	std::string id;
	std::string name;
	std::string description;
	std::vector<DatasetField> fields;
};

void from_json(const nlohmann::json& j, DatasetField& f);
void from_json(const nlohmann::json& j, DatasetMetadata& m);

} // namespace ncei
