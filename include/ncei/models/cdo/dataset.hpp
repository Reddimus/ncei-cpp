#pragma once

#include <string>

namespace ncei {

struct Dataset {
	std::string id; // "GHCND", "GSOM", etc.
	std::string name;
	double data_coverage{0.0};
	std::string min_date;
	std::string max_date;
};

} // namespace ncei
