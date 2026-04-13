#include "ncei/cdo_client.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(CDOClientConfigTest, DefaultBaseUrl) {
	CDOClient::Config config;
	config.token = "test-token";
	EXPECT_TRUE(config.http.base_url.empty());
	// Constructor should set default base URL
	CDOClient client(std::move(config));
	EXPECT_EQ(client.http_client().config().base_url, "https://www.ncei.noaa.gov/cdo-web/api/v2");
}

TEST(CDOClientConfigTest, CustomBaseUrl) {
	CDOClient::Config config;
	config.token = "test-token";
	config.http.base_url = "https://custom.example.com/api";
	CDOClient client(std::move(config));
	EXPECT_EQ(client.http_client().config().base_url, "https://custom.example.com/api");
}

TEST(CDOClientConfigTest, TokenInHeaders) {
	CDOClient::Config config;
	config.token = "my-secret-token";
	CDOClient client(std::move(config));

	bool found = false;
	for (const std::pair<std::string, std::string>& header :
		 client.http_client().config().extra_headers) {
		if (header.first == "token" && header.second == "my-secret-token") {
			found = true;
			break;
		}
	}
	EXPECT_TRUE(found);
}

TEST(CDOClientConfigTest, DefaultRateLimitConfig) {
	CDOClient::Config config;
	config.token = "test-token";
	EXPECT_EQ(config.rate_limit.max_tokens, 5);
	EXPECT_EQ(config.rate_limit.daily_limit, 10000);
}

TEST(CDOClientConfigTest, DefaultRetryPolicy) {
	CDOClient::Config config;
	config.token = "test-token";
	EXPECT_EQ(config.retry.max_attempts, 3);
	EXPECT_TRUE(config.retry.retry_on_rate_limit);
	EXPECT_TRUE(config.retry.retry_on_server_error);
}

TEST(CDOClientConfigTest, DailyRequestsRemaining) {
	CDOClient::Config config;
	config.token = "test-token";
	CDOClient client(std::move(config));
	EXPECT_EQ(client.daily_requests_remaining(), 10000);
}

TEST(CDOListParamsTest, AllOptional) {
	CDOListParams params;
	EXPECT_FALSE(params.dataset_id.has_value());
	EXPECT_FALSE(params.location_id.has_value());
	EXPECT_FALSE(params.station_id.has_value());
	EXPECT_FALSE(params.data_type_id.has_value());
	EXPECT_FALSE(params.data_category_id.has_value());
	EXPECT_FALSE(params.location_category_id.has_value());
	EXPECT_FALSE(params.start_date.has_value());
	EXPECT_FALSE(params.end_date.has_value());
	EXPECT_FALSE(params.sort_field.has_value());
	EXPECT_FALSE(params.sort_order.has_value());
	EXPECT_FALSE(params.limit.has_value());
	EXPECT_FALSE(params.offset.has_value());
}

TEST(GetDataParamsTest, RequiredFields) {
	GetDataParams params;
	params.dataset_id = "GHCND";
	params.start_date = "2024-01-01";
	params.end_date = "2024-01-31";
	EXPECT_EQ(params.dataset_id, "GHCND");
	EXPECT_EQ(params.start_date, "2024-01-01");
	EXPECT_EQ(params.end_date, "2024-01-31");
}

TEST(GetDataParamsTest, OptionalFields) {
	GetDataParams params;
	params.dataset_id = "GHCND";
	params.start_date = "2024-01-01";
	params.end_date = "2024-01-31";
	EXPECT_FALSE(params.data_type_ids.has_value());
	EXPECT_FALSE(params.location_id.has_value());
	EXPECT_FALSE(params.station_id.has_value());
	EXPECT_FALSE(params.units.has_value());
	EXPECT_FALSE(params.include_metadata.has_value());
}

TEST(CDOClientTest, MoveConstruction) {
	CDOClient::Config config;
	config.token = "test-token";
	CDOClient client1(std::move(config));

	CDOClient client2(std::move(client1));
	EXPECT_EQ(client2.http_client().config().base_url, "https://www.ncei.noaa.gov/cdo-web/api/v2");
}

} // namespace
} // namespace ncei
