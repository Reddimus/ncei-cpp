#include "ncei/date_range.hpp"

#include <chrono>
#include <format>
#include <stdexcept>

namespace ncei {

namespace {

std::chrono::sys_days to_sys_days(const DateParts& parts) {
	std::chrono::year_month_day ymd{std::chrono::year{parts.year},
									std::chrono::month{static_cast<unsigned>(parts.month)},
									std::chrono::day{static_cast<unsigned>(parts.day)}};
	return std::chrono::sys_days{ymd};
}

DateParts from_sys_days(std::chrono::sys_days sd) {
	std::chrono::year_month_day ymd{sd};
	DateParts parts;
	parts.year = static_cast<std::int32_t>(ymd.year());
	parts.month = static_cast<std::int32_t>(static_cast<unsigned>(ymd.month()));
	parts.day = static_cast<std::int32_t>(static_cast<unsigned>(ymd.day()));
	return parts;
}

} // namespace

DateParts parse_date(const std::string& iso_date) {
	DateParts parts;
	parts.year = std::stoi(iso_date.substr(0, 4));
	parts.month = std::stoi(iso_date.substr(5, 2));
	parts.day = std::stoi(iso_date.substr(8, 2));
	return parts;
}

std::string format_date(const DateParts& parts) {
	return std::format("{:04d}-{:02d}-{:02d}", parts.year, parts.month, parts.day);
}

std::int32_t days_between(const std::string& start, const std::string& end) {
	DateParts start_parts = parse_date(start);
	DateParts end_parts = parse_date(end);
	std::chrono::sys_days start_days = to_sys_days(start_parts);
	std::chrono::sys_days end_days = to_sys_days(end_parts);
	std::chrono::days diff = end_days - start_days;
	return static_cast<std::int32_t>(diff.count());
}

bool date_range_exceeds(const std::string& start, const std::string& end, std::int32_t max_days) {
	return days_between(start, end) > max_days;
}

std::vector<DateRange> split_date_range(const std::string& start, const std::string& end,
										std::int32_t max_days) {
	std::vector<DateRange> ranges;
	std::string chunk_start = start;

	while (true) {
		std::int32_t remaining = days_between(chunk_start, end);
		if (remaining <= max_days) {
			ranges.emplace_back(chunk_start, end);
			break;
		}
		std::string chunk_end = add_days(chunk_start, max_days);
		ranges.emplace_back(chunk_start, chunk_end);
		chunk_start = add_days(chunk_end, 1);
	}

	return ranges;
}

std::string add_days(const std::string& date, std::int32_t days) {
	DateParts parts = parse_date(date);
	std::chrono::sys_days sd = to_sys_days(parts);
	sd += std::chrono::days{days};
	DateParts result = from_sys_days(sd);
	return format_date(result);
}

} // namespace ncei
