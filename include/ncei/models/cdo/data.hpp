#pragma once

#include <string>

namespace ncei {

struct DataRecord {
	std::string date;
	std::string datatype; // "TMAX", "TMIN", etc.
	std::string station;
	std::string attributes;
	double value{0.0};
};

} // namespace ncei
