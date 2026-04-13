#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>

namespace ncei {

std::string json_string(const nlohmann::json& j, const char* key) {
	if (j.contains(key) && j[key].is_string()) {
		return j[key].get<std::string>();
	}
	return {};
}

std::int32_t json_int(const nlohmann::json& j, const char* key, std::int32_t def) {
	if (j.contains(key) && j[key].is_number()) {
		return j[key].get<std::int32_t>();
	}
	return def;
}

double json_double(const nlohmann::json& j, const char* key, double def) {
	if (j.contains(key) && j[key].is_number()) {
		return j[key].get<double>();
	}
	return def;
}

bool json_bool(const nlohmann::json& j, const char* key, bool def) {
	if (j.contains(key) && j[key].is_boolean()) {
		return j[key].get<bool>();
	}
	return def;
}

} // namespace ncei
