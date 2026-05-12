// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Glaze-deserializer tests — verify the migration's parse output matches
// the documented NCEI response shape and the pre-migration behavior
// (null-safety for missing/null scalar fields, snake_case ↔ camelCase
// JSON-key aliasing, dynamic-key DataPoint attribute preservation,
// CDO list-response envelope handling).

#include "ncei/models/cdo/data.hpp"
#include "ncei/models/cdo/dataset.hpp"
#include "ncei/models/cdo/station.hpp"
#include "ncei/models/common.hpp"
#include "ncei/models/data_service/data_point.hpp"
#include "ncei/pagination.hpp"

#include <gtest/gtest.h>
#include <string>

namespace ncei {
namespace {

TEST(GlazeDeserializerTest, ParsesStation) {
	const std::string body = R"({
		"id": "GHCND:USW00013874",
		"name": "RALEIGH DURHAM INTL AP",
		"datacoverage": 1.0,
		"mindate": "1948-01-01",
		"maxdate": "2024-12-31",
		"latitude": 35.8917,
		"longitude": -78.7833,
		"elevation": 124.4,
		"elevationUnit": "METERS"
	})";

	CDOStation s;
	Result<void> r = deserialize_station(body, s);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_EQ(s.id, "GHCND:USW00013874");
	EXPECT_EQ(s.elevation_unit, "METERS");
	EXPECT_DOUBLE_EQ(s.latitude, 35.8917);
}

TEST(GlazeDeserializerTest, TolerantOfUnknownKeys) {
	// CDO occasionally adds new metadata keys (e.g. `parentStationId`); we
	// must keep parsing.
	const std::string body = R"({
		"id": "GHCND:USW00013874",
		"name": "RALEIGH",
		"datacoverage": 1.0,
		"mindate": "1948-01-01",
		"maxdate": "2024-12-31",
		"latitude": 35.89,
		"longitude": -78.78,
		"elevation": 124.4,
		"elevationUnit": "METERS",
		"parentStationId": "GHCND:US0000",
		"thirdParty": {"foo": "bar"}
	})";
	CDOStation s;
	Result<void> r = deserialize_station(body, s);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_EQ(s.id, "GHCND:USW00013874");
}

TEST(GlazeDeserializerTest, NullScalarsLeaveDefaults) {
	const std::string body = R"({
		"id": "GHCND:USW00013874",
		"name": null,
		"datacoverage": null,
		"mindate": null,
		"maxdate": null,
		"latitude": null,
		"longitude": null,
		"elevation": null,
		"elevationUnit": null
	})";
	CDOStation s;
	Result<void> r = deserialize_station(body, s);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_EQ(s.id, "GHCND:USW00013874");
	EXPECT_TRUE(s.name.empty());
	EXPECT_DOUBLE_EQ(s.data_coverage, 0.0);
	EXPECT_DOUBLE_EQ(s.latitude, 0.0);
	EXPECT_TRUE(s.elevation_unit.empty());
}

TEST(GlazeDeserializerTest, ParsesCdoListEnvelopeWithDatasets) {
	const std::string body = R"({
		"metadata": {"resultset": {"offset": 0, "count": 2, "limit": 25}},
		"results": [
			{"id": "GHCND", "name": "Daily Summaries", "datacoverage": 1.0,
			 "mindate": "1763-01-01", "maxdate": "2024-12-31"},
			{"id": "GSOM", "name": "Monthly Summaries", "datacoverage": 1.0,
			 "mindate": "1763-01-01", "maxdate": "2024-11-01"}
		]
	})";

	CDOResponse<Dataset> resp;
	Result<void> r = deserialize_cdo_list(body, resp);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_EQ(resp.metadata.offset, 0);
	EXPECT_EQ(resp.metadata.count, 2);
	EXPECT_EQ(resp.metadata.limit, 25);
	ASSERT_EQ(resp.results.size(), 2u);
	EXPECT_EQ(resp.results[0].id, "GHCND");
	EXPECT_EQ(resp.results[1].id, "GSOM");
}

TEST(GlazeDeserializerTest, ParsesDataPointCollectionWithDynamicKeys) {
	// DataPoint has user-driven keys (TMAX/TMIN/PRCP/SNOW from the query
	// string). Verify they land in attributes, and the known scalars in
	// their typed slots.
	const std::string body = R"([
		{
			"DATE": "2024-01-15",
			"STATION": "USW00013874",
			"NAME": "RALEIGH",
			"LATITUDE": 35.89,
			"LONGITUDE": -78.78,
			"ELEVATION": 124.0,
			"TMAX": "122",
			"TMIN": "44",
			"PRCP": "0.0"
		}
	])";
	DataPointCollection dpc;
	Result<void> r = deserialize_data_point_collection(body, dpc);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	ASSERT_EQ(dpc.records.size(), 1u);
	const DataPoint& dp = dpc.records[0];
	EXPECT_EQ(dp.date, "2024-01-15");
	EXPECT_EQ(dp.station, "USW00013874");
	EXPECT_DOUBLE_EQ(dp.latitude, 35.89);

	// Dynamic keys land in attributes, NOT in the typed scalar slots
	std::optional<std::string> tmax = dp.get("TMAX");
	ASSERT_TRUE(tmax.has_value());
	EXPECT_EQ(*tmax, "122");
	std::optional<std::string> prcp = dp.get("PRCP");
	ASSERT_TRUE(prcp.has_value());
	EXPECT_EQ(*prcp, "0.0");
}

TEST(GlazeDeserializerTest, DataPointCollectionPreservesNumericAttributesAsStrings) {
	// CDO occasionally emits numeric (not string) values for measurements;
	// we should stringify them so the attribute map is uniform.
	const std::string body = R"([
		{"DATE": "2024-01-15", "STATION": "X", "NAME": "Y",
		 "LATITUDE": 0.0, "LONGITUDE": 0.0, "ELEVATION": 0.0,
		 "TMAX": 122}
	])";
	DataPointCollection dpc;
	Result<void> r = deserialize_data_point_collection(body, dpc);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	ASSERT_EQ(dpc.records.size(), 1u);
	std::optional<std::string> tmax = dpc.records[0].get("TMAX");
	ASSERT_TRUE(tmax.has_value());
	EXPECT_FALSE(tmax->empty());
}

TEST(GlazeDeserializerTest, ParsesDataRecord) {
	const std::string body = R"({
		"date": "2024-01-15T00:00:00",
		"datatype": "TMAX",
		"station": "GHCND:USW00013874",
		"attributes": ",,N,2400",
		"value": 122
	})";
	DataRecord d;
	Result<void> r = deserialize_data_record(body, d);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_EQ(d.date, "2024-01-15T00:00:00");
	EXPECT_EQ(d.datatype, "TMAX");
	EXPECT_DOUBLE_EQ(d.value, 122.0);
}

TEST(GlazeDeserializerTest, RejectsMalformedJson) {
	const std::string body = R"({"id": "GHCND")"; // unterminated
	Dataset d;
	Result<void> r = deserialize_dataset(body, d);
	EXPECT_FALSE(r.has_value());
}

TEST(GlazeDeserializerTest, CdoListEnvelopeHandlesEmptyResults) {
	const std::string body = R"({
		"metadata": {"resultset": {"offset": 0, "count": 0, "limit": 25}},
		"results": []
	})";
	CDOResponse<Dataset> resp;
	Result<void> r = deserialize_cdo_list(body, resp);
	ASSERT_TRUE(r.has_value()) << (r ? "" : r.error().message);
	EXPECT_TRUE(resp.results.empty());
	EXPECT_EQ(resp.metadata.count, 0);
}

} // namespace
} // namespace ncei
