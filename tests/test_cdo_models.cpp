#include "ncei/models/cdo/data.hpp"
#include "ncei/models/cdo/data_category.hpp"
#include "ncei/models/cdo/data_type.hpp"
#include "ncei/models/cdo/dataset.hpp"
#include "ncei/models/cdo/location.hpp"
#include "ncei/models/cdo/location_category.hpp"
#include "ncei/models/cdo/station.hpp"
#include "ncei/pagination.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace ncei {
namespace {

// --- Dataset ---

TEST(DatasetTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "GHCND",
		"name": "Daily Summaries",
		"datacoverage": 1.0,
		"mindate": "1763-01-01",
		"maxdate": "2024-12-31"
	})");
	Dataset d;
	from_json(j, d);
	EXPECT_EQ(d.id, "GHCND");
	EXPECT_EQ(d.name, "Daily Summaries");
	EXPECT_DOUBLE_EQ(d.data_coverage, 1.0);
	EXPECT_EQ(d.min_date, "1763-01-01");
	EXPECT_EQ(d.max_date, "2024-12-31");
}

TEST(DatasetTest, DefaultConstruction) {
	Dataset d;
	EXPECT_TRUE(d.id.empty());
	EXPECT_TRUE(d.name.empty());
	EXPECT_DOUBLE_EQ(d.data_coverage, 0.0);
	EXPECT_TRUE(d.min_date.empty());
	EXPECT_TRUE(d.max_date.empty());
}

TEST(DatasetTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null,
		"name": null,
		"datacoverage": null,
		"mindate": null,
		"maxdate": null
	})");
	Dataset d;
	from_json(j, d);
	EXPECT_TRUE(d.id.empty());
	EXPECT_TRUE(d.name.empty());
	EXPECT_DOUBLE_EQ(d.data_coverage, 0.0);
}

// --- DataCategory ---

TEST(DataCategoryTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "TEMP",
		"name": "Air Temperature"
	})");
	DataCategory dc;
	from_json(j, dc);
	EXPECT_EQ(dc.id, "TEMP");
	EXPECT_EQ(dc.name, "Air Temperature");
}

TEST(DataCategoryTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({"id": null, "name": null})");
	DataCategory dc;
	from_json(j, dc);
	EXPECT_TRUE(dc.id.empty());
	EXPECT_TRUE(dc.name.empty());
}

// --- DataType ---

TEST(DataTypeTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "TMAX",
		"name": "Maximum temperature",
		"datacoverage": 0.95,
		"mindate": "1900-01-01",
		"maxdate": "2024-12-31"
	})");
	DataType dt;
	from_json(j, dt);
	EXPECT_EQ(dt.id, "TMAX");
	EXPECT_EQ(dt.name, "Maximum temperature");
	EXPECT_DOUBLE_EQ(dt.data_coverage, 0.95);
	EXPECT_EQ(dt.min_date, "1900-01-01");
	EXPECT_EQ(dt.max_date, "2024-12-31");
}

TEST(DataTypeTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null, "name": null, "datacoverage": null,
		"mindate": null, "maxdate": null
	})");
	DataType dt;
	from_json(j, dt);
	EXPECT_TRUE(dt.id.empty());
	EXPECT_DOUBLE_EQ(dt.data_coverage, 0.0);
}

// --- LocationCategory ---

TEST(LocationCategoryTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "ST",
		"name": "State"
	})");
	LocationCategory lc;
	from_json(j, lc);
	EXPECT_EQ(lc.id, "ST");
	EXPECT_EQ(lc.name, "State");
}

TEST(LocationCategoryTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({"id": null, "name": null})");
	LocationCategory lc;
	from_json(j, lc);
	EXPECT_TRUE(lc.id.empty());
	EXPECT_TRUE(lc.name.empty());
}

// --- Location ---

TEST(LocationTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "FIPS:37",
		"name": "North Carolina",
		"datacoverage": 1.0,
		"mindate": "1869-03-01",
		"maxdate": "2024-12-31"
	})");
	Location l;
	from_json(j, l);
	EXPECT_EQ(l.id, "FIPS:37");
	EXPECT_EQ(l.name, "North Carolina");
	EXPECT_DOUBLE_EQ(l.data_coverage, 1.0);
	EXPECT_EQ(l.min_date, "1869-03-01");
	EXPECT_EQ(l.max_date, "2024-12-31");
}

TEST(LocationTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null, "name": null, "datacoverage": null,
		"mindate": null, "maxdate": null
	})");
	Location l;
	from_json(j, l);
	EXPECT_TRUE(l.id.empty());
	EXPECT_DOUBLE_EQ(l.data_coverage, 0.0);
}

// --- CDOStation ---

TEST(CDOStationTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "GHCND:USW00013874",
		"name": "RALEIGH DURHAM INTERNATIONAL AIRPORT, NC US",
		"datacoverage": 1.0,
		"mindate": "1948-01-01",
		"maxdate": "2024-12-31",
		"latitude": 35.8917,
		"longitude": -78.7833,
		"elevation": 124.4,
		"elevationUnit": "METERS"
	})");
	CDOStation s;
	from_json(j, s);
	EXPECT_EQ(s.id, "GHCND:USW00013874");
	EXPECT_EQ(s.name, "RALEIGH DURHAM INTERNATIONAL AIRPORT, NC US");
	EXPECT_DOUBLE_EQ(s.data_coverage, 1.0);
	EXPECT_EQ(s.min_date, "1948-01-01");
	EXPECT_EQ(s.max_date, "2024-12-31");
	EXPECT_DOUBLE_EQ(s.latitude, 35.8917);
	EXPECT_DOUBLE_EQ(s.longitude, -78.7833);
	EXPECT_DOUBLE_EQ(s.elevation, 124.4);
	EXPECT_EQ(s.elevation_unit, "METERS");
}

TEST(CDOStationTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null, "name": null, "datacoverage": null,
		"mindate": null, "maxdate": null,
		"latitude": null, "longitude": null,
		"elevation": null, "elevationUnit": null
	})");
	CDOStation s;
	from_json(j, s);
	EXPECT_TRUE(s.id.empty());
	EXPECT_DOUBLE_EQ(s.latitude, 0.0);
	EXPECT_DOUBLE_EQ(s.longitude, 0.0);
	EXPECT_DOUBLE_EQ(s.elevation, 0.0);
	EXPECT_TRUE(s.elevation_unit.empty());
}

// --- DataRecord ---

TEST(DataRecordTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"date": "2024-01-15T00:00:00",
		"datatype": "TMAX",
		"station": "GHCND:USW00013874",
		"attributes": ",,N,2400",
		"value": 122
	})");
	DataRecord d;
	from_json(j, d);
	EXPECT_EQ(d.date, "2024-01-15T00:00:00");
	EXPECT_EQ(d.datatype, "TMAX");
	EXPECT_EQ(d.station, "GHCND:USW00013874");
	EXPECT_EQ(d.attributes, ",,N,2400");
	EXPECT_DOUBLE_EQ(d.value, 122.0);
}

TEST(DataRecordTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"date": null, "datatype": null, "station": null,
		"attributes": null, "value": null
	})");
	DataRecord d;
	from_json(j, d);
	EXPECT_TRUE(d.date.empty());
	EXPECT_TRUE(d.datatype.empty());
	EXPECT_DOUBLE_EQ(d.value, 0.0);
}

// --- CDOResponse ---

TEST(CDOResponseDatasetTest, FromJsonWithMetadata) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"metadata": {
			"resultset": {
				"offset": 0,
				"count": 2,
				"limit": 25
			}
		},
		"results": [
			{
				"id": "GHCND",
				"name": "Daily Summaries",
				"datacoverage": 1.0,
				"mindate": "1763-01-01",
				"maxdate": "2024-12-31"
			},
			{
				"id": "GSOM",
				"name": "Global Summary of the Month",
				"datacoverage": 1.0,
				"mindate": "1763-01-01",
				"maxdate": "2024-11-01"
			}
		]
	})");

	CDOResponse<Dataset> resp;
	from_json(j, resp.metadata);

	EXPECT_EQ(resp.metadata.offset, 0);
	EXPECT_EQ(resp.metadata.count, 2);
	EXPECT_EQ(resp.metadata.limit, 25);

	if (j.contains("results") && j["results"].is_array()) {
		for (const nlohmann::json& item : j["results"]) {
			Dataset ds;
			from_json(item, ds);
			resp.results.push_back(std::move(ds));
		}
	}

	ASSERT_EQ(resp.results.size(), 2u);
	EXPECT_EQ(resp.results[0].id, "GHCND");
	EXPECT_EQ(resp.results[1].id, "GSOM");
}

TEST(CDOResponseTest, HasMoreTrue) {
	CDOResponse<Dataset> resp;
	resp.metadata.offset = 0;
	resp.metadata.limit = 25;
	resp.metadata.count = 100;
	EXPECT_TRUE(resp.has_more());
}

TEST(CDOResponseTest, HasMoreFalseAtEnd) {
	CDOResponse<Dataset> resp;
	resp.metadata.offset = 75;
	resp.metadata.limit = 25;
	resp.metadata.count = 100;
	EXPECT_FALSE(resp.has_more());
}

// --- Missing fields ---

TEST(DatasetTest, MissingFields) {
	nlohmann::json j = nlohmann::json::parse(R"({"id": "GHCND"})");
	Dataset d;
	from_json(j, d);
	EXPECT_EQ(d.id, "GHCND");
	EXPECT_TRUE(d.name.empty());
	EXPECT_DOUBLE_EQ(d.data_coverage, 0.0);
	EXPECT_TRUE(d.min_date.empty());
	EXPECT_TRUE(d.max_date.empty());
}

} // namespace
} // namespace ncei
