#pragma once

#include <string>

namespace ncei {

struct LocationCategory {
	std::string id; // "CITY", "ST", "CNTRY", etc.
	std::string name;
};

} // namespace ncei
