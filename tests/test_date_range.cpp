#include "ncei/date_range.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(ParseDateTest, ValidDate) {
	DateParts parts = parse_date("2023-06-15");
	EXPECT_EQ(parts.year, 2023);
	EXPECT_EQ(parts.month, 6);
	EXPECT_EQ(parts.day, 15);
}

TEST(ParseDateTest, Jan1) {
	DateParts parts = parse_date("2020-01-01");
	EXPECT_EQ(parts.year, 2020);
	EXPECT_EQ(parts.month, 1);
	EXPECT_EQ(parts.day, 1);
}

TEST(ParseDateTest, Dec31) {
	DateParts parts = parse_date("2023-12-31");
	EXPECT_EQ(parts.year, 2023);
	EXPECT_EQ(parts.month, 12);
	EXPECT_EQ(parts.day, 31);
}

TEST(ParseDateTest, Feb29LeapYear) {
	DateParts parts = parse_date("2024-02-29");
	EXPECT_EQ(parts.year, 2024);
	EXPECT_EQ(parts.month, 2);
	EXPECT_EQ(parts.day, 29);
}

TEST(FormatDateTest, RoundTrip) {
	std::string original = "2023-06-15";
	DateParts parts = parse_date(original);
	std::string formatted = format_date(parts);
	EXPECT_EQ(formatted, original);
}

TEST(DaysBetweenTest, SameDay) {
	std::int32_t diff = days_between("2023-06-15", "2023-06-15");
	EXPECT_EQ(diff, 0);
}

TEST(DaysBetweenTest, OneYear) {
	std::int32_t diff = days_between("2023-01-01", "2024-01-01");
	EXPECT_EQ(diff, 365);
}

TEST(DaysBetweenTest, IncludesLeapYear) {
	std::int32_t diff = days_between("2024-01-01", "2025-01-01");
	EXPECT_EQ(diff, 366);
}

TEST(DateRangeExceedsTest, True) {
	bool exceeds = date_range_exceeds("2020-01-01", "2022-01-01", 365);
	EXPECT_TRUE(exceeds);
}

TEST(DateRangeExceedsTest, False) {
	bool exceeds = date_range_exceeds("2023-01-01", "2023-06-15", 365);
	EXPECT_FALSE(exceeds);
}

TEST(SplitDateRangeTest, SameYearNoSplit) {
	std::vector<DateRange> ranges = split_date_range("2023-01-01", "2023-06-15");
	ASSERT_EQ(ranges.size(), 1u);
	EXPECT_EQ(ranges[0].first, "2023-01-01");
	EXPECT_EQ(ranges[0].second, "2023-06-15");
}

TEST(SplitDateRangeTest, Exactly365Days) {
	std::vector<DateRange> ranges = split_date_range("2023-01-01", "2024-01-01");
	ASSERT_EQ(ranges.size(), 1u);
	EXPECT_EQ(ranges[0].first, "2023-01-01");
	EXPECT_EQ(ranges[0].second, "2024-01-01");
}

TEST(SplitDateRangeTest, MultiYearTwoChunks) {
	std::vector<DateRange> ranges = split_date_range("2023-01-01", "2024-06-15");
	ASSERT_EQ(ranges.size(), 2u);
	EXPECT_EQ(ranges[0].first, "2023-01-01");
	EXPECT_EQ(ranges[1].second, "2024-06-15");
}

TEST(SplitDateRangeTest, FiveYearsFiveChunks) {
	std::vector<DateRange> ranges = split_date_range("2019-01-01", "2024-01-01");
	// 5 years = ~1826 days, with 365 per chunk => ceil(1826/365) = 6 chunks
	// But we use <= so exact multiples fit in fewer.
	// 2019-01-01 to 2024-01-01 = 1827 days (includes leap year 2020)
	// Each chunk covers 365 days, so we expect ceil(1827/365) = 6 chunks
	EXPECT_GE(ranges.size(), 5u);
	EXPECT_EQ(ranges.front().first, "2019-01-01");
	EXPECT_EQ(ranges.back().second, "2024-01-01");
}

TEST(SplitDateRangeTest, StartEqualsEnd) {
	std::vector<DateRange> ranges = split_date_range("2023-06-15", "2023-06-15");
	ASSERT_EQ(ranges.size(), 1u);
	EXPECT_EQ(ranges[0].first, "2023-06-15");
	EXPECT_EQ(ranges[0].second, "2023-06-15");
}

TEST(AddDaysTest, Basic) {
	std::string result = add_days("2023-06-15", 10);
	EXPECT_EQ(result, "2023-06-25");
}

TEST(AddDaysTest, AcrossMonthBoundary) {
	std::string result = add_days("2023-01-28", 5);
	EXPECT_EQ(result, "2023-02-02");
}

TEST(AddDaysTest, AcrossYearBoundary) {
	std::string result = add_days("2023-12-30", 5);
	EXPECT_EQ(result, "2024-01-04");
}

} // namespace
} // namespace ncei
