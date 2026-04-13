#include "ncei/error.hpp"

#include <gtest/gtest.h>

namespace ncei {
namespace {

TEST(ErrorCodeTest, ToStringCoversAllCodes) {
	EXPECT_EQ(to_string(ErrorCode::Ok), "Ok");
	EXPECT_EQ(to_string(ErrorCode::NetworkError), "NetworkError");
	EXPECT_EQ(to_string(ErrorCode::RateLimited), "RateLimited");
	EXPECT_EQ(to_string(ErrorCode::QuotaExceeded), "QuotaExceeded");
	EXPECT_EQ(to_string(ErrorCode::ServerError), "ServerError");
	EXPECT_EQ(to_string(ErrorCode::NotFound), "NotFound");
	EXPECT_EQ(to_string(ErrorCode::InvalidRequest), "InvalidRequest");
	EXPECT_EQ(to_string(ErrorCode::ParseError), "ParseError");
	EXPECT_EQ(to_string(ErrorCode::CacheError), "CacheError");
	EXPECT_EQ(to_string(ErrorCode::Unknown), "Unknown");
}

TEST(ErrorTest, OkFactory) {
	Error err = Error::ok();
	EXPECT_TRUE(err.is_ok());
	EXPECT_EQ(err.code, ErrorCode::Ok);
	EXPECT_TRUE(err.message.empty());
}

TEST(ErrorTest, NetworkFactory) {
	Error err = Error::network("connection refused");
	EXPECT_FALSE(err.is_ok());
	EXPECT_EQ(err.code, ErrorCode::NetworkError);
	EXPECT_EQ(err.message, "connection refused");
}

TEST(ErrorTest, ParseFactory) {
	Error err = Error::parse("invalid JSON");
	EXPECT_EQ(err.code, ErrorCode::ParseError);
	EXPECT_EQ(err.message, "invalid JSON");
}

TEST(ErrorTest, NotFoundFactory) {
	Error err = Error::not_found("dataset not found");
	EXPECT_EQ(err.code, ErrorCode::NotFound);
	EXPECT_EQ(err.message, "dataset not found");
}

TEST(ErrorTest, RateLimitedFactory) {
	Error err = Error::rate_limited("too many requests");
	EXPECT_EQ(err.code, ErrorCode::RateLimited);
	EXPECT_EQ(err.message, "too many requests");
}

TEST(ErrorTest, QuotaExceededFactory) {
	Error err = Error::quota_exceeded("daily quota exceeded");
	EXPECT_EQ(err.code, ErrorCode::QuotaExceeded);
	EXPECT_EQ(err.message, "daily quota exceeded");
}

TEST(ErrorTest, ServerFactory) {
	Error err = Error::server("internal server error");
	EXPECT_EQ(err.code, ErrorCode::ServerError);
	EXPECT_EQ(err.message, "internal server error");
}

TEST(ErrorTest, InvalidRequestFactory) {
	Error err = Error::invalid_request("bad parameter");
	EXPECT_EQ(err.code, ErrorCode::InvalidRequest);
	EXPECT_EQ(err.message, "bad parameter");
}

TEST(ErrorTest, FromResponseCdoFormat) {
	std::string body = R"({"status":"400","message":"Invalid token"})";
	Error err = Error::from_response(400, body);
	EXPECT_EQ(err.code, ErrorCode::InvalidRequest);
	EXPECT_EQ(err.http_status, 400);
	EXPECT_EQ(err.message, "Invalid token");
}

TEST(ErrorTest, FromResponseDataServiceFormat) {
	std::string body = R"({"errorMessage":"Bad request","errors":["invalid station"]})";
	Error err = Error::from_response(400, body);
	EXPECT_EQ(err.code, ErrorCode::InvalidRequest);
	EXPECT_EQ(err.http_status, 400);
	EXPECT_EQ(err.message, "Bad request");
	EXPECT_EQ(err.detail, "invalid station");
}

TEST(ErrorTest, FromResponseDataServiceMultipleErrors) {
	std::string body =
		R"({"errorMessage":"Bad request","errors":["invalid station","missing date"]})";
	Error err = Error::from_response(400, body);
	EXPECT_EQ(err.message, "Bad request");
	EXPECT_EQ(err.detail, "invalid station; missing date");
}

TEST(ErrorTest, FromResponseRfc7807) {
	std::string body = R"({
		"type": "https://api.ncei.noaa.gov/problems/InvalidRequest",
		"title": "Invalid Request",
		"status": 404,
		"detail": "Unable to provide data for requested dataset",
		"correlationId": "abc123"
	})";

	Error err = Error::from_response(404, body);
	EXPECT_EQ(err.code, ErrorCode::NotFound);
	EXPECT_EQ(err.http_status, 404);
	EXPECT_EQ(err.message, "Invalid Request");
	EXPECT_EQ(err.detail, "Unable to provide data for requested dataset");
	EXPECT_EQ(err.correlation_id, "abc123");
}

TEST(ErrorTest, FromResponsePlainText) {
	Error err = Error::from_response(500, "Internal Server Error");
	EXPECT_EQ(err.code, ErrorCode::ServerError);
	EXPECT_EQ(err.http_status, 500);
	EXPECT_EQ(err.message, "Internal Server Error");
}

TEST(ErrorTest, FromResponse429IsRateLimited) {
	Error err = Error::from_response(429, "{}");
	EXPECT_EQ(err.code, ErrorCode::RateLimited);
}

TEST(ErrorTest, FromResponse400IsInvalidRequest) {
	Error err = Error::from_response(400, "{}");
	EXPECT_EQ(err.code, ErrorCode::InvalidRequest);
}

TEST(ErrorTest, FromResponseWithCorrelationIdParam) {
	Error err = Error::from_response(500, "{}", "xyz789");
	EXPECT_EQ(err.correlation_id, "xyz789");
}

TEST(ResultTest, ValueAccess) {
	Result<int> result = 42;
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, 42);
}

TEST(ResultTest, ErrorAccess) {
	Result<int> result = std::unexpected(Error::network("fail"));
	ASSERT_FALSE(result.has_value());
	EXPECT_EQ(result.error().code, ErrorCode::NetworkError);
	EXPECT_EQ(result.error().message, "fail");
}

TEST(ResultTest, StringValue) {
	Result<std::string> result = std::string("hello");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, "hello");
}

} // namespace
} // namespace ncei
