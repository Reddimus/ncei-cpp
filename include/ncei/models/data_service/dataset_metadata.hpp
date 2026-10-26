#pragma once
#include <string>
#include <vector>

namespace ncei {

struct DatasetField {
	std::string id;
	std::string name;
	std::string description;
	std::string data_type; // "string", "number", etc.
	std::string units;
	double scale_factor{1.0};
};

struct DatasetMetadata {
	std::string id;
	std::string name;
	std::string description;
	std::vector<DatasetField> fields; // dataTypes in Access support/v3 responses
};

} // namespace ncei
