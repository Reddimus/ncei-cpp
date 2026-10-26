#include "ncei/cache.hpp"
#include "ncei/cdo_client.hpp"
#include "ncei/csv_parser.hpp"
#include "ncei/date_range.hpp"
#include "ncei/models/cdo/dataset.hpp"
#include "ncei/models/climate_index.hpp"
#include "ncei/models/common.hpp"
#include "ncei/models/data_service/data_point.hpp"
#include "ncei/models/data_service/dataset_metadata.hpp"
#include "ncei/monitoring_client.hpp"
#include "ncei/pagination.hpp"
#include "ncei/rate_limit.hpp"
#include "ncei/retry.hpp"

#include <gtest/gtest.h>
#include <limits>
#include <memory>

namespace ncei {
TEST(Regression, OneBasedPaginationIncludesTheLastRecord) {
	CDOResponse<int> response;
	response.metadata = {.offset = 1, .count = 26, .limit = 25};
	EXPECT_TRUE(response.has_more());
	EXPECT_EQ(response.next_offset(), 26);
}

TEST(Regression, PaginationArithmeticCannotOverflow) {
	CDOResponse<int> response;
	response.metadata = {.offset = std::numeric_limits<std::int32_t>::max(),
						 .count = std::numeric_limits<std::int32_t>::max(),
						 .limit = 25};
	EXPECT_FALSE(response.has_more());
	EXPECT_EQ(response.next_offset(), std::numeric_limits<std::int32_t>::max());
}

TEST(Regression, CompleteJsonAndFreshOutputAreRequired) {
	Dataset out;
	ASSERT_TRUE(deserialize_dataset(R"({"id":"old","datacoverage":0.5})", out));
	ASSERT_TRUE(deserialize_dataset("{}", out));
	EXPECT_TRUE(out.id.empty());
	EXPECT_FALSE(deserialize_dataset("{} trailing", out));
	EXPECT_FALSE(deserialize_dataset(R"({"unknown":tru})", out));
}

TEST(Regression, BorrowedJsonBuffersNeedNoTerminator) {
	const std::string input = R"({"unknown":[true,false,null,{"s":"\u1234"}]})";
	for (std::size_t n = 1; n <= input.size(); ++n) {
		std::unique_ptr<char[]> bytes = std::make_unique<char[]>(n);
		std::copy_n(input.data(), n, bytes.get());
		Dataset out;
		EXPECT_EQ(deserialize_dataset({bytes.get(), n}, out).has_value(), n == input.size());
	}
}

TEST(Regression, ZeroWaitConsumesAvailableTokens) {
	RateLimiter limiter({.max_tokens = 1, .initial_tokens = 1});
	EXPECT_TRUE(limiter.acquire_for(std::chrono::milliseconds::zero()));
}

TEST(Regression, RetryDelayRespectsCap) {
	RetryPolicy policy{.initial_delay = std::chrono::milliseconds{100},
					   .max_delay = std::chrono::milliseconds{100},
					   .jitter_factor = 1};
	for (int i = 0; i < 100; ++i) {
		EXPECT_LE(calculate_retry_delay(2, policy), policy.max_delay);
	}
}
} // namespace ncei

namespace ncei {
TEST(Regression, InvalidDateRangesFailWithoutLooping) {
	EXPECT_THROW((void)parse_date("2023-02-29"), std::invalid_argument);
	EXPECT_THROW((void)parse_date("2024-01-01extra"), std::invalid_argument);
	EXPECT_THROW((void)split_date_range("2024-01-01", "2024-01-02", -1), std::invalid_argument);
	EXPECT_THROW((void)split_date_range("2024-02-01", "2024-01-01"), std::invalid_argument);
	EXPECT_THROW((void)add_days("9999-12-31", 1), std::out_of_range);
}
TEST(Regression, ZeroCapacityAndExtremeTtlAreSafe) {
	LruCache<int, int> disabled({.max_entries = 0});
	disabled.put(1, 2);
	EXPECT_EQ(disabled.size(), 0);
	LruCache<int, int> cache({.max_entries = 1, .ttl = std::chrono::seconds::max()});
	cache.put(1, 2);
	EXPECT_EQ(cache.get(1), 2);
}
struct ThrowingMove {
	inline static bool fail = false;
	ThrowingMove() = default;
	ThrowingMove(const ThrowingMove&) = default;
	ThrowingMove& operator=(const ThrowingMove&) = default;
	ThrowingMove(ThrowingMove&&) {
		if (fail) {
			throw std::runtime_error("test move");
		}
	}
	ThrowingMove& operator=(ThrowingMove&&) {
		if (fail) {
			throw std::runtime_error("test move");
		}
		return *this;
	}
};
TEST(Regression, FailedCacheInsertionPreservesLruOrder) {
	LruCache<int, ThrowingMove> cache({.max_entries = 2});
	ThrowingMove value;
	ThrowingMove::fail = true;
	EXPECT_THROW(cache.put(1, value), std::runtime_error);
	ThrowingMove::fail = false;
	cache.put(1, value);
	cache.put(2, value);
	ASSERT_TRUE(cache.get(1));
	cache.put(3, value);
	EXPECT_TRUE(cache.get(1));
	EXPECT_FALSE(cache.get(2));
}
} // namespace ncei

namespace ncei {
TEST(Regression, DateComponentsCannotWrapInChronoTypes) {
	EXPECT_THROW((void)format_date({.year = 2024, .month = 257, .day = 257}),
				 std::invalid_argument);
	EXPECT_THROW((void)format_date({.year = 2024, .month = -255, .day = 1}), std::invalid_argument);
}
TEST(Regression, FinalQuotedCsvFieldDoesNotAddAColumn) {
	const DelimitedParser parser(DelimitedParser::Delimiter::Comma);
	EXPECT_EQ(parser.parse_line("a,\"b\""), (std::vector<std::string>{"a", "b"}));
	EXPECT_EQ(parser.parse_line("a,\"b\","), (std::vector<std::string>{"a", "b", ""}));
}
TEST(Regression, ClimateOutputsReplaceDataAndPreserveOnFailure) {
	CagSeries series;
	ASSERT_TRUE(deserialize_cag_series(R"({"data":{"2024":"1.1"}})", series));
	EXPECT_FALSE(deserialize_cag_series(R"({"data":{}})", series));
	ASSERT_EQ(series.data.size(), 1);
	ASSERT_TRUE(deserialize_cag_series(R"({"data":{"2025":"1.2"}})", series));
	ASSERT_EQ(series.data.size(), 1);
	EXPECT_EQ(series.data.front().period, "2025");
	EXPECT_FALSE(deserialize_cag_series(R"({"data":{"2024":"1.2junk"}})", series));
	EXPECT_FALSE(deserialize_cag_series(R"({"data":{"2024":"NaN"}})", series));
}
TEST(Regression, ShortCsvRowsCannotReadPastTheirColumns) {
	GistempSeries gistemp;
	EXPECT_FALSE(deserialize_gistemp_csv("J-D,Other,Year\n1.2,x\n", gistemp));
	SeaIceSeries ice;
	EXPECT_FALSE(deserialize_nsidc_sea_ice_csv("extent,other,year\n12.3,x\n", true, ice));
}
TEST(Regression, InvalidNumericCellsAreNotPartiallyParsed) {
	DataPoint point;
	point.attributes = {{"TMAX", "1.2junk"}};
	EXPECT_FALSE(point.get_double("TMAX"));
	point.attributes = {{"TMAX", "inf"}};
	EXPECT_FALSE(point.get_double("TMAX"));
	point.attributes = {{"TMAX", " +1.2 "}};
	EXPECT_EQ(point.get_double("TMAX"), 1.2);
}
TEST(Regression, MovedFromClientsAndInvalidMonitoringInputsFailSafely) {
	CDOClient first(CDOClient::Config{});
	CDOClient second(std::move(first));
	EXPECT_EQ(first.daily_requests_remaining(), 0);
	EXPECT_FALSE(first.get_datasets({}));
	MonitoringClient monitoring(MonitoringClient::Config{});
	EXPECT_FALSE(monitoring.get_cag_global_annual(2025, 2024));
	EXPECT_FALSE(monitoring.get_nsidc_arctic_extent(true, 13));
}
} // namespace ncei

namespace ncei {
TEST(Regression, AccessMetadataMapsDataTypesAndScaling) {
	DatasetMetadata metadata;
	ASSERT_TRUE(deserialize_dataset_metadata(
		R"({"id":"daily-summaries","dataTypes":[{"id":"TMAX","units":"celsius","scaleFactor":0.1}]})",
		metadata));
	ASSERT_EQ(metadata.fields.size(), 1);
	EXPECT_EQ(metadata.fields.front().id, "TMAX");
	EXPECT_EQ(metadata.fields.front().units, "celsius");
	EXPECT_DOUBLE_EQ(metadata.fields.front().scale_factor, 0.1);
}
} // namespace ncei

namespace ncei {
TEST(Regression, QuotedCsvNewlinesStayWithinTheirRecord) {
	const DelimitedParser parser(DelimitedParser::Delimiter::Comma);
	const std::vector<std::vector<std::string>> rows = parser.parse("a,\"b\nc\"\nx,y\n");
	ASSERT_EQ(rows.size(), 2);
	EXPECT_EQ(rows.front(), (std::vector<std::string>{"a", "b\nc"}));
	EXPECT_EQ(parser.parse_header("a,\"b\nc\"\nx,y"), rows.front());
}
} // namespace ncei

namespace ncei {
TEST(Regression, DailySeaIceFlagAndFailureOutputArePreserved) {
	SeaIceSeries out;
	ASSERT_TRUE(
		deserialize_nsidc_sea_ice_csv("Year,Month,Day,Extent\n2024,1,2,12.5\n", false, out));
	EXPECT_FALSE(out.monthly);
	ASSERT_EQ(out.extent.size(), 1);
	EXPECT_FALSE(deserialize_nsidc_sea_ice_csv("", true, out));
	EXPECT_FALSE(out.monthly);
	EXPECT_EQ(out.extent.size(), 1);
}
} // namespace ncei

namespace ncei {
TEST(Regression, NumericTextRejectsMultipleSigns) {
	for (const std::string& text : {"+-1.2", "++1.2", "+", "--1.2", "-+1.2"}) {
		DataPoint point;
		point.attributes.emplace_back("TMAX", text);
		EXPECT_FALSE(point.get_double("TMAX")) << text;
		CagSeries series;
		EXPECT_FALSE(deserialize_cag_series("{\"data\":{\"2024\":\"" + text + "\"}}", series))
			<< text;
	}
	DataPoint point;
	point.attributes.emplace_back("TMAX", "+1.2");
	EXPECT_EQ(point.get_double("TMAX"), 1.2);
	CagSeries series;
	ASSERT_TRUE(deserialize_cag_series(R"({"data":{"2024":"+1.2"}})", series));
	ASSERT_EQ(series.data.size(), 1);
	EXPECT_EQ(series.data.front().value, 1.2);
}
} // namespace ncei
