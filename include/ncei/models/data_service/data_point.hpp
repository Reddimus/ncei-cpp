#pragma once
#include <cstdint>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ncei {

struct DataPoint {
	std::string date;
	std::string station;
	std::string name; // station name
	double latitude{0.0};
	double longitude{0.0};
	double elevation{0.0};
	std::vector<std::pair<std::string, std::string>> attributes;

	[[nodiscard]] std::optional<std::string> get(std::string_view key) const;
	[[nodiscard]] std::optional<double> get_double(std::string_view key) const;
};

struct DataPointCollection {
	std::vector<std::string> columns;
	std::vector<DataPoint> records;
};

void from_json(const nlohmann::json& j, DataPoint& dp);
void from_json(const nlohmann::json& j, DataPointCollection& dpc);

[[nodiscard]] DataPointCollection parse_csv_data(std::string_view csv_text);
[[nodiscard]] DataPointCollection parse_ssv_data(std::string_view ssv_text);

} // namespace ncei
