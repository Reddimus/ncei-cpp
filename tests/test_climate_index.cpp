// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/climate_index.hpp"

#include <gtest/gtest.h>
#include <string>

namespace ncei {
namespace {

TEST(ClimateIndex, ParsesCagJson) {
	const std::string body = R"({"description":{"title":"Global Land and Ocean",)"
							 R"("units":"Degrees Celsius","base_period":"1901-2000"},)"
							 R"("data":{"1880":"-0.19","2024":"+1.29","2025":"+1.26"}})";
	CagSeries s;
	Result<void> r = deserialize_cag_series(body, s);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	EXPECT_EQ(s.units, "Degrees Celsius");
	EXPECT_EQ(s.base_period, "1901-2000");
	ASSERT_EQ(s.data.size(), 3u);
	bool found_2025 = false;
	for (const ClimatePoint& p : s.data) {
		if (p.period == "2025") {
			found_2025 = true;
			EXPECT_DOUBLE_EQ(p.value, 1.26);
		}
	}
	EXPECT_TRUE(found_2025);
}

TEST(ClimateIndex, RejectsBadCag) {
	CagSeries s;
	EXPECT_FALSE(deserialize_cag_series("not json", s).has_value());
	EXPECT_FALSE(deserialize_cag_series(R"({"description":{}})", s).has_value());
}

TEST(ClimateIndex, ParsesGistempCsv) {
	const std::string csv =
		"Land-Ocean Temperature Index (C)\n"
		"--------------------------------\n"
		"Year,Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec,J-D,D-N\n"
		"1880,-.19,-.25,-.10,-.16,-.10,-.21,-.18,-.10,-.15,-.23,-.22,-.18,-.16,***\n"
		"2024,1.25,1.40,1.30,1.27,1.20,1.22,1.24,1.28,1.30,1.40,1.30,1.25,1.28,1.27\n";
	GistempSeries s;
	Result<void> r = deserialize_gistemp_csv(csv, s);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	ASSERT_EQ(s.annual.size(), 2u);
	EXPECT_EQ(s.annual[0].period, "1880");
	EXPECT_DOUBLE_EQ(s.annual[0].value, -0.16);
	EXPECT_EQ(s.annual[1].period, "2024");
	EXPECT_DOUBLE_EQ(s.annual[1].value, 1.28);
}

TEST(ClimateIndex, ParsesNsidcMonthlyCsv) {
	const std::string csv = "year, mo, data-type, region, extent, area\n"
							"1979, 9, Goddard, N, 7.05, 4.58\n"
							"2012, 9, Goddard, N, 3.57, 2.41\n"
							"2024, 9, NRTSI-G, N, 4.28, -9999\n";
	SeaIceSeries s;
	Result<void> r = deserialize_nsidc_sea_ice_csv(csv, true, s);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	EXPECT_TRUE(s.monthly);
	ASSERT_EQ(s.extent.size(), 3u);
	EXPECT_EQ(s.extent[0].period, "1979-09");
	EXPECT_DOUBLE_EQ(s.extent[0].value, 7.05);
	EXPECT_EQ(s.extent[1].period, "2012-09");
	EXPECT_DOUBLE_EQ(s.extent[1].value, 3.57);
}

TEST(ClimateIndex, RejectsBadNsidc) {
	SeaIceSeries s;
	EXPECT_FALSE(deserialize_nsidc_sea_ice_csv("", true, s).has_value());
	EXPECT_FALSE(deserialize_nsidc_sea_ice_csv("foo,bar\n1,2\n", true, s).has_value());
}

} // namespace
} // namespace ncei
