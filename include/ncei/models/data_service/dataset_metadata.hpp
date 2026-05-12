#pragma once
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

} // namespace ncei
