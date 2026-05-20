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

// 2026-05-19: NCEI CAG response format flipped from `{"1851": "-0.12"}`
// (bare string) to `{"1851": {"departure": -0.12}}` (nested object). Our
// pre-flip deserializer treated the nested-object form as non-numeric,
// dropping every row → "CAG: empty data series" + 0 ingested anomalies.
// The fix tolerates both forms; keep regression tests for both so a
// future flip doesn't re-break us silently.
TEST(ClimateIndex, ParsesCagDepartureObjectForm) {
	// Numeric departure values inside per-year objects, plus an extra
	// inner key to confirm we ignore unrelated fields gracefully.
	const std::string body = R"({"description":{"title":"Global Land and Ocean",)"
							 R"("units":"Degrees Celsius","base_period":"1901-2000"},)"
							 R"("data":{"1880":{"departure":-0.19},)"
							 R"("2024":{"departure":1.29,"rank":1},)"
							 R"("2025":{"departure":1.26}}})";
	CagSeries s;
	const Result<void> r = deserialize_cag_series(body, s);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	EXPECT_EQ(s.units, "Degrees Celsius");
	EXPECT_EQ(s.base_period, "1901-2000");
	ASSERT_EQ(s.data.size(), 3u);
	bool found_2024 = false;
	bool found_2025 = false;
	for (const ClimatePoint& p : s.data) {
		if (p.period == "2024") {
			found_2024 = true;
			EXPECT_DOUBLE_EQ(p.value, 1.29);
		} else if (p.period == "2025") {
			found_2025 = true;
			EXPECT_DOUBLE_EQ(p.value, 1.26);
		}
	}
	EXPECT_TRUE(found_2024);
	EXPECT_TRUE(found_2025);
}

TEST(ClimateIndex, ParsesCagDepartureStringInsideObject) {
	// Some NCEI products may keep the string-form departure inside the
	// new object envelope; we accept that too.
	const std::string body = R"({"description":{"units":"Degrees Celsius"},)"
							 R"("data":{"1850":{"departure":"-0.10"},)"
							 R"("1851":{"departure":"+0.05"}}})";
	CagSeries s;
	const Result<void> r = deserialize_cag_series(body, s);
	ASSERT_TRUE(r.has_value()) << r.error().message;
	ASSERT_EQ(s.data.size(), 2u);
	bool found_1851 = false;
	for (const ClimatePoint& p : s.data) {
		if (p.period == "1851") {
			found_1851 = true;
			EXPECT_DOUBLE_EQ(p.value, 0.05);
		}
	}
	EXPECT_TRUE(found_1851);
}

TEST(ClimateIndex, RejectsCagObjectWithoutDeparture) {
	// If the object envelope is present but has no "departure" key, we
	// treat that row as missing rather than coerce it into something
	// arbitrary. Pre-fix the empty-series error would surface; we
	// preserve that failure mode for "no usable rows" so an operator
	// sees the upstream change immediately.
	const std::string body = R"({"description":{},"data":{)"
							 R"("1851":{"other_field":1.2},)"
							 R"("1852":{"rank":5}}})";
	CagSeries s;
	const Result<void> r = deserialize_cag_series(body, s);
	EXPECT_FALSE(r.has_value());
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
