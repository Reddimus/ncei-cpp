#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ncei {

using DateRange = std::pair<std::string, std::string>;

struct DateParts {
	std::int32_t year{0};
	std::int32_t month{0};
	std::int32_t day{0};
};

[[nodiscard]] DateParts parse_date(const std::string& iso_date);
[[nodiscard]] std::string format_date(const DateParts& parts);
[[nodiscard]] std::int32_t days_between(const std::string& start, const std::string& end);
[[nodiscard]] bool date_range_exceeds(const std::string& start, const std::string& end,
									  std::int32_t max_days);
[[nodiscard]] std::vector<DateRange>
split_date_range(const std::string& start, const std::string& end, std::int32_t max_days = 365);
[[nodiscard]] std::string add_days(const std::string& date, std::int32_t days);

} // namespace ncei
