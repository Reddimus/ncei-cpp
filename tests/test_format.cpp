#include "ncei/format.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(FormatTest, ToStringAllFormats) {
	EXPECT_EQ(to_string(ResponseFormat::CSV), "csv");
	EXPECT_EQ(to_string(ResponseFormat::JSON), "json");
	EXPECT_EQ(to_string(ResponseFormat::SSV), "ssv");
	EXPECT_EQ(to_string(ResponseFormat::PDF), "pdf");
	EXPECT_EQ(to_string(ResponseFormat::NetCDF), "netcdf");
}

TEST(FormatTest, ContentTypeAllFormats) {
	EXPECT_EQ(content_type(ResponseFormat::CSV), "text/csv");
	EXPECT_EQ(content_type(ResponseFormat::JSON), "application/json");
	EXPECT_EQ(content_type(ResponseFormat::SSV), "text/plain");
	EXPECT_EQ(content_type(ResponseFormat::PDF), "application/pdf");
	EXPECT_EQ(content_type(ResponseFormat::NetCDF), "application/x-netcdf");
}

TEST(FormatTest, ToStringIsConstexpr) {
	constexpr std::string_view csv = to_string(ResponseFormat::CSV);
	EXPECT_EQ(csv, "csv");
}

TEST(FormatTest, ContentTypeIsConstexpr) {
	constexpr std::string_view json_ct = content_type(ResponseFormat::JSON);
	EXPECT_EQ(json_ct, "application/json");
}

} // namespace
} // namespace ncei
