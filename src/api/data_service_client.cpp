#include "ncei/data_service_client.hpp"

#include "ncei/models/common.hpp"

#include <format>
#include <iterator>
#include <nlohmann/json.hpp>
#include <utility>

namespace ncei {

namespace {

/// URL-encode a date string, replacing '+' with '%2B' for timezone offsets
std::string url_encode_date(const std::string& date) {
	std::string result;
	result.reserve(date.size() + 4);
	for (char c : date) {
		if (c == '+') {
			result += "%2B";
		} else {
			result += c;
		}
	}
	return result;
}

} // namespace

struct DataServiceClient::Impl {
	HttpClient http;
	RetryPolicy retry;

	Impl(ClientConfig http_config, RetryPolicy rp)
		: http(std::move(http_config)), retry(std::move(rp)) {}
};

DataServiceClient::DataServiceClient(Config config) {
	if (config.http.base_url.empty()) {
		config.http.base_url = "https://www.ncei.noaa.gov/access/services";
	}

	impl_ = std::make_unique<Impl>(std::move(config.http), config.retry);
}

DataServiceClient::~DataServiceClient() = default;
DataServiceClient::DataServiceClient(DataServiceClient&&) noexcept = default;
DataServiceClient& DataServiceClient::operator=(DataServiceClient&&) noexcept = default;

HttpClient& DataServiceClient::http_client() {
	return impl_->http;
}

const HttpClient& DataServiceClient::http_client() const {
	return impl_->http;
}

Result<HttpResponse> DataServiceClient::do_get(std::string_view path) {
	return with_retry([&]() -> Result<HttpResponse> { return impl_->http.get(path); },
					  impl_->retry);
}

std::string DataServiceClient::build_data_query(const DataRequestParams& params) {
	std::string url = std::format("/data/v1?dataset={}", params.dataset);
	url.reserve(url.size() + 256);
	std::back_insert_iterator<std::string> out(url);

	if (params.start_date) {
		std::format_to(out, "&startDate={}", url_encode_date(*params.start_date));
	}
	if (params.end_date) {
		std::format_to(out, "&endDate={}", url_encode_date(*params.end_date));
	}
	if (params.stations) {
		bool first = true;
		for (const std::string& station : *params.stations) {
			std::format_to(out, "{}{}", first ? "&stations=" : ",", station);
			first = false;
		}
	}
	if (params.data_types) {
		bool first = true;
		for (const std::string& dt : *params.data_types) {
			std::format_to(out, "{}{}", first ? "&dataTypes=" : ",", dt);
			first = false;
		}
	}
	if (params.format)
		std::format_to(out, "&format={}", to_string(*params.format));
	if (params.units)
		std::format_to(out, "&units={}", *params.units);
	if (params.bbox)
		std::format_to(out, "&boundingBox={}", *params.bbox);
	if (params.include_attributes) {
		std::format_to(out, "&includeAttributes={}", *params.include_attributes ? "true" : "false");
	}
	if (params.include_station_name) {
		std::format_to(out, "&includeStationName={}",
					   *params.include_station_name ? "true" : "false");
	}
	if (params.include_station_location) {
		std::format_to(out, "&includeStationLocation={}",
					   *params.include_station_location ? "true" : "false");
	}

	return url;
}

Result<DataPointCollection> DataServiceClient::get_data(const DataRequestParams& params) {
	std::string query = build_data_query(params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}

	ResponseFormat fmt = params.format.value_or(ResponseFormat::CSV);

	try {
		switch (fmt) {
			case ResponseFormat::CSV:
				return parse_csv_data(response->body);
			case ResponseFormat::JSON: {
				nlohmann::json j = nlohmann::json::parse(response->body);
				DataPointCollection dpc;
				from_json(j, dpc);
				return dpc;
			}
			case ResponseFormat::SSV:
				return parse_ssv_data(response->body);
			case ResponseFormat::PDF:
			case ResponseFormat::NetCDF:
				return std::unexpected(Error::invalid_request(
					"PDF/NetCDF formats cannot be parsed as DataPointCollection; "
					"use get_data_raw() instead"));
		}
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(std::string("JSON parse error: ") + e.what()));
	}

	return std::unexpected(Error::invalid_request("Unknown response format"));
}

Result<std::string> DataServiceClient::get_data_raw(const DataRequestParams& params) {
	std::string query = build_data_query(params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return std::move(response->body);
}

Result<DatasetMetadata> DataServiceClient::get_dataset_metadata(const std::string& dataset_id) {
	std::string path = "/support/v3/datasets/" + dataset_id + ".json";
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}

	try {
		nlohmann::json j = nlohmann::json::parse(response->body);
		DatasetMetadata meta;
		from_json(j, meta);
		return meta;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(std::string("JSON parse error: ") + e.what()));
	}
}

} // namespace ncei
