#include "ncei/pagination.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace ncei {
namespace {

TEST(ResultSetMetadataTest, DefaultConstruction) {
	ResultSetMetadata m;
	EXPECT_EQ(m.offset, 0);
	EXPECT_EQ(m.count, 0);
	EXPECT_EQ(m.limit, 25);
}

TEST(ResultSetMetadataTest, FromJson) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"metadata": {
			"resultset": {
				"offset": 50,
				"count": 200,
				"limit": 25
			}
		}
	})");
	ResultSetMetadata m;
	from_json(j, m);
	EXPECT_EQ(m.offset, 50);
	EXPECT_EQ(m.count, 200);
	EXPECT_EQ(m.limit, 25);
}

TEST(ResultSetMetadataTest, FromJsonNestedStructure) {
	nlohmann::json j = nlohmann::json::parse(R"({
		"metadata": {
			"resultset": {
				"offset": 0,
				"count": 100,
				"limit": 25
			}
		}
	})");
	ResultSetMetadata m;
	from_json(j, m);
	EXPECT_EQ(m.offset, 0);
	EXPECT_EQ(m.count, 100);
	EXPECT_EQ(m.limit, 25);
}

TEST(CDOResponseTest, HasMoreTrue) {
	CDOResponse<int> resp;
	resp.metadata.offset = 0;
	resp.metadata.limit = 25;
	resp.metadata.count = 100;
	EXPECT_TRUE(resp.has_more());
}

TEST(CDOResponseTest, HasMoreFalse) {
	CDOResponse<int> resp;
	resp.metadata.offset = 75;
	resp.metadata.limit = 25;
	resp.metadata.count = 100;
	EXPECT_FALSE(resp.has_more());
}

TEST(CDOResponseTest, NextOffset) {
	CDOResponse<int> resp;
	resp.metadata.offset = 25;
	resp.metadata.limit = 25;
	EXPECT_EQ(resp.next_offset(), 50);
}

TEST(CDOResponseTest, EmptyResults) {
	CDOResponse<std::string> resp;
	EXPECT_TRUE(resp.results.empty());
	EXPECT_FALSE(resp.has_more());
}

TEST(CDOResponseTest, SinglePage) {
	CDOResponse<int> resp;
	resp.metadata.offset = 0;
	resp.metadata.limit = 25;
	resp.metadata.count = 10;
	EXPECT_FALSE(resp.has_more());
}

} // namespace
} // namespace ncei
