#pragma once

#include <string>

namespace ncei {

struct Location {
	std::string id; // "FIPS:37", "CITY:US...", etc.
	std::string name;
	double data_coverage{0.0};
	std::string min_date;
	std::string max_date;
};

} // namespace ncei
