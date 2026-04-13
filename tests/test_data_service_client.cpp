#include "ncei/data_service_client.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(DataServiceClientConfigTest, DefaultBaseUrl) {
	DataServiceClient::Config config;
	EXPECT_TRUE(config.http.base_url.empty());
	DataServiceClient client(std::move(config));
	EXPECT_EQ(client.http_client().config().base_url, "https://www.ncei.noaa.gov/access/services");
}

TEST(DataServiceClientConfigTest, CustomBaseUrl) {
	DataServiceClient::Config config;
	config.http.base_url = "https://custom.example.com/api";
	DataServiceClient client(std::move(config));
	EXPECT_EQ(client.http_client().config().base_url, "https://custom.example.com/api");
}

TEST(DataRequestParamsTest, Defaults) {
	DataRequestParams params;
	EXPECT_TRUE(params.dataset.empty());
	EXPECT_FALSE(params.start_date.has_value());
	EXPECT_FALSE(params.end_date.has_value());
	EXPECT_FALSE(params.stations.has_value());
	EXPECT_FALSE(params.data_types.has_value());
	EXPECT_FALSE(params.format.has_value());
	EXPECT_FALSE(params.units.has_value());
	EXPECT_FALSE(params.bbox.has_value());
	EXPECT_FALSE(params.include_attributes.has_value());
	EXPECT_FALSE(params.include_station_name.has_value());
	EXPECT_FALSE(params.include_station_location.has_value());
}

TEST(DataSearchParamsTest, Defaults) {
	DataSearchParams params;
	EXPECT_TRUE(params.dataset.empty());
	EXPECT_FALSE(params.start_date.has_value());
	EXPECT_FALSE(params.end_date.has_value());
	EXPECT_FALSE(params.stations.has_value());
	EXPECT_FALSE(params.data_types.has_value());
	EXPECT_FALSE(params.bbox.has_value());
	EXPECT_FALSE(params.text.has_value());
}

TEST(DataServiceClientConfigTest, DefaultRetryPolicy) {
	DataServiceClient::Config config;
	EXPECT_EQ(config.retry.max_attempts, 3);
	EXPECT_TRUE(config.retry.retry_on_server_error);
}

TEST(DataServiceClientTest, MoveConstruction) {
	DataServiceClient::Config config;
	DataServiceClient client1(std::move(config));
	DataServiceClient client2(std::move(client1));
	EXPECT_EQ(client2.http_client().config().base_url, "https://www.ncei.noaa.gov/access/services");
}

TEST(DataServiceClientTest, MoveAssignment) {
	DataServiceClient::Config config1;
	DataServiceClient client1(std::move(config1));

	DataServiceClient::Config config2;
	config2.http.base_url = "https://other.example.com";
	DataServiceClient client2(std::move(config2));

	client2 = std::move(client1);
	EXPECT_EQ(client2.http_client().config().base_url, "https://www.ncei.noaa.gov/access/services");
}

} // namespace
} // namespace ncei
