#pragma once

#include <string>

namespace ncei {

struct DataCategory {
	std::string id; // "TEMP", "PRECIP", etc.
	std::string name;
};

} // namespace ncei
