#pragma once

#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ncei {

[[nodiscard]] std::string json_string(const nlohmann::json& j, const char* key);
[[nodiscard]] std::int32_t json_int(const nlohmann::json& j, const char* key, std::int32_t def = 0);
[[nodiscard]] double json_double(const nlohmann::json& j, const char* key, double def = 0.0);
[[nodiscard]] bool json_bool(const nlohmann::json& j, const char* key, bool def = false);

} // namespace ncei
