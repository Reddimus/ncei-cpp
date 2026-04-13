#include "ncei/models/data_service/data_point.hpp"
#include "ncei/models/data_service/dataset_metadata.hpp"
#include "ncei/models/data_service/search_result.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace ncei {
namespace {

// --- DataPoint from JSON ---

TEST(DataPointTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"DATE": "2024-01-15",
		"STATION": "USW00013874",
		"NAME": "RALEIGH DURHAM INTL AP, NC US",
		"LATITUDE": 35.8917,
		"LONGITUDE": -78.7833,
		"ELEVATION": 124.4,
		"TMAX": "122",
		"TMIN": "44"
	})");
	DataPoint dp;
	from_json(j, dp);
	EXPECT_EQ(dp.date, "2024-01-15");
	EXPECT_EQ(dp.station, "USW00013874");
	EXPECT_EQ(dp.name, "RALEIGH DURHAM INTL AP, NC US");
	EXPECT_DOUBLE_EQ(dp.latitude, 35.8917);
	EXPECT_DOUBLE_EQ(dp.longitude, -78.7833);
	EXPECT_DOUBLE_EQ(dp.elevation, 124.4);
	ASSERT_GE(dp.attributes.size(), 2u);
}

// --- DataPoint get() ---

TEST(DataPointTest, GetAttribute) {
	DataPoint dp;
	dp.attributes.emplace_back("TMAX", "122");
	dp.attributes.emplace_back("TMIN", "44");

	std::optional<std::string> val = dp.get("TMAX");
	ASSERT_TRUE(val.has_value());
	EXPECT_EQ(*val, "122");
}

TEST(DataPointTest, GetMissingAttribute) {
	DataPoint dp;
	dp.attributes.emplace_back("TMAX", "122");

	std::optional<std::string> val = dp.get("PRCP");
	EXPECT_FALSE(val.has_value());
}

// --- DataPoint get_double() ---

TEST(DataPointTest, GetDoubleValid) {
	DataPoint dp;
	dp.attributes.emplace_back("TMAX", "12.5");

	std::optional<double> val = dp.get_double("TMAX");
	ASSERT_TRUE(val.has_value());
	EXPECT_DOUBLE_EQ(*val, 12.5);
}

TEST(DataPointTest, GetDoubleNonNumeric) {
	DataPoint dp;
	dp.attributes.emplace_back("FLAG", "abc");

	std::optional<double> val = dp.get_double("FLAG");
	EXPECT_FALSE(val.has_value());
}

TEST(DataPointTest, GetDoubleMissing) {
	DataPoint dp;
	std::optional<double> val = dp.get_double("TMAX");
	EXPECT_FALSE(val.has_value());
}

// --- DataPointCollection from JSON array ---

TEST(DataPointCollectionTest, FromJsonArray) {
	nlohmann::json j = nlohmann::json::parse(R"([
		{
			"DATE": "2024-01-15",
			"STATION": "USW00013874",
			"NAME": "RALEIGH",
			"LATITUDE": 35.89,
			"LONGITUDE": -78.78,
			"ELEVATION": 124.0,
			"TMAX": "122"
		},
		{
			"DATE": "2024-01-16",
			"STATION": "USW00013874",
			"NAME": "RALEIGH",
			"LATITUDE": 35.89,
			"LONGITUDE": -78.78,
			"ELEVATION": 124.0,
			"TMAX": "130"
		}
	])");
	DataPointCollection dpc;
	from_json(j, dpc);
	EXPECT_FALSE(dpc.columns.empty());
	ASSERT_EQ(dpc.records.size(), 2u);
	EXPECT_EQ(dpc.records[0].date, "2024-01-15");
	EXPECT_EQ(dpc.records[1].date, "2024-01-16");
}

// --- parse_csv_data ---

TEST(ParseCsvDataTest, BasicCsv) {
	std::string csv = "DATE,STATION,NAME,LATITUDE,LONGITUDE,ELEVATION,TMAX,TMIN\n"
					  "2024-01-15,USW00013874,RALEIGH,35.89,-78.78,124.0,122,44\n"
					  "2024-01-16,USW00013874,RALEIGH,35.89,-78.78,124.0,130,50\n";

	DataPointCollection dpc = parse_csv_data(csv);
	ASSERT_EQ(dpc.columns.size(), 8u);
	EXPECT_EQ(dpc.columns[0], "DATE");
	EXPECT_EQ(dpc.columns[6], "TMAX");
	ASSERT_EQ(dpc.records.size(), 2u);
	EXPECT_EQ(dpc.records[0].date, "2024-01-15");
	EXPECT_EQ(dpc.records[0].station, "USW00013874");
	EXPECT_DOUBLE_EQ(dpc.records[0].latitude, 35.89);

	std::optional<std::string> tmax = dpc.records[0].get("TMAX");
	ASSERT_TRUE(tmax.has_value());
	EXPECT_EQ(*tmax, "122");
}

TEST(ParseCsvDataTest, StationLocationColumns) {
	std::string csv = "DATE,STATION,NAME,LATITUDE,LONGITUDE,ELEVATION,TMAX\n"
					  "2024-01-15,USW00013874,RALEIGH,35.89,-78.78,124.0,122\n";

	DataPointCollection dpc = parse_csv_data(csv);
	ASSERT_EQ(dpc.records.size(), 1u);
	EXPECT_EQ(dpc.records[0].name, "RALEIGH");
	EXPECT_DOUBLE_EQ(dpc.records[0].latitude, 35.89);
	EXPECT_DOUBLE_EQ(dpc.records[0].longitude, -78.78);
	EXPECT_DOUBLE_EQ(dpc.records[0].elevation, 124.0);
}

TEST(ParseCsvDataTest, EmptyCsv) {
	DataPointCollection dpc = parse_csv_data("");
	EXPECT_TRUE(dpc.columns.empty());
	EXPECT_TRUE(dpc.records.empty());
}

TEST(ParseCsvDataTest, QuotedFields) {
	std::string csv = "DATE,STATION,NAME,LATITUDE,LONGITUDE,ELEVATION,TMAX\n"
					  "2024-01-15,USW00013874,\"RALEIGH, NC US\",35.89,-78.78,124.0,122\n";

	DataPointCollection dpc = parse_csv_data(csv);
	ASSERT_EQ(dpc.records.size(), 1u);
	EXPECT_EQ(dpc.records[0].name, "RALEIGH, NC US");
}

// --- parse_ssv_data ---

TEST(ParseSsvDataTest, BasicSsv) {
	std::string ssv = "DATE STATION TMAX\n"
					  "2024-01-15 USW00013874 122\n";

	DataPointCollection dpc = parse_ssv_data(ssv);
	ASSERT_EQ(dpc.columns.size(), 3u);
	ASSERT_EQ(dpc.records.size(), 1u);
	EXPECT_EQ(dpc.records[0].date, "2024-01-15");
	EXPECT_EQ(dpc.records[0].station, "USW00013874");
}

// --- DataSearchResult ---

TEST(DataSearchResultTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "USW00013874",
		"name": "RALEIGH DURHAM INTL AP",
		"latitude": 35.8917,
		"longitude": -78.7833,
		"elevation": 124.4,
		"mindate": "1948-01-01",
		"maxdate": "2024-12-31",
		"datacoverage": 1.0
	})");
	DataSearchResult r;
	from_json(j, r);
	EXPECT_EQ(r.station_id, "USW00013874");
	EXPECT_EQ(r.station_name, "RALEIGH DURHAM INTL AP");
	EXPECT_DOUBLE_EQ(r.latitude, 35.8917);
	EXPECT_DOUBLE_EQ(r.longitude, -78.7833);
	EXPECT_DOUBLE_EQ(r.elevation, 124.4);
	EXPECT_EQ(r.min_date, "1948-01-01");
	EXPECT_EQ(r.max_date, "2024-12-31");
	EXPECT_DOUBLE_EQ(r.data_coverage, 1.0);
}

// --- DatasetSearchResult ---

TEST(DatasetSearchResultTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"uid": "gov.noaa.ncdc:C00861",
		"name": "Daily Summaries",
		"description": "Global daily weather data",
		"startDate": "1763-01-01",
		"endDate": "2024-12-31",
		"dataTypes": ["TMAX", "TMIN", "PRCP"]
	})");
	DatasetSearchResult r;
	from_json(j, r);
	EXPECT_EQ(r.uid, "gov.noaa.ncdc:C00861");
	EXPECT_EQ(r.name, "Daily Summaries");
	EXPECT_EQ(r.description, "Global daily weather data");
	EXPECT_EQ(r.start_date, "1763-01-01");
	EXPECT_EQ(r.end_date, "2024-12-31");
	ASSERT_EQ(r.data_types.size(), 3u);
	EXPECT_EQ(r.data_types[0], "TMAX");
}

// --- DatasetField ---

TEST(DatasetFieldTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "TMAX",
		"name": "Maximum Temperature",
		"description": "Maximum temperature for the day",
		"dataType": "number"
	})");
	DatasetField f;
	from_json(j, f);
	EXPECT_EQ(f.id, "TMAX");
	EXPECT_EQ(f.name, "Maximum Temperature");
	EXPECT_EQ(f.description, "Maximum temperature for the day");
	EXPECT_EQ(f.data_type, "number");
}

// --- DatasetMetadata ---

TEST(DatasetMetadataTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": "daily-summaries",
		"name": "Daily Summaries",
		"description": "Global daily weather observations",
		"fields": [
			{
				"id": "TMAX",
				"name": "Maximum Temperature",
				"description": "Max temp",
				"dataType": "number"
			},
			{
				"id": "TMIN",
				"name": "Minimum Temperature",
				"description": "Min temp",
				"dataType": "number"
			}
		]
	})");
	DatasetMetadata m;
	from_json(j, m);
	EXPECT_EQ(m.id, "daily-summaries");
	EXPECT_EQ(m.name, "Daily Summaries");
	EXPECT_EQ(m.description, "Global daily weather observations");
	ASSERT_EQ(m.fields.size(), 2u);
	EXPECT_EQ(m.fields[0].id, "TMAX");
	EXPECT_EQ(m.fields[1].id, "TMIN");
}

// --- Null safety ---

TEST(DataSearchResultTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null, "name": null, "latitude": null, "longitude": null,
		"elevation": null, "mindate": null, "maxdate": null, "datacoverage": null
	})");
	DataSearchResult r;
	from_json(j, r);
	EXPECT_TRUE(r.station_id.empty());
	EXPECT_TRUE(r.station_name.empty());
	EXPECT_DOUBLE_EQ(r.latitude, 0.0);
	EXPECT_DOUBLE_EQ(r.data_coverage, 0.0);
}

TEST(DatasetMetadataTest, NullFields) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"id": null, "name": null, "description": null
	})");
	DatasetMetadata m;
	from_json(j, m);
	EXPECT_TRUE(m.id.empty());
	EXPECT_TRUE(m.name.empty());
	EXPECT_TRUE(m.fields.empty());
}

} // namespace
} // namespace ncei
