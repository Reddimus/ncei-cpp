#include "ncei/data_service_client.hpp"

#include "ncei/models/common.hpp"

#include <format>
#include <iterator>
#include <stdexcept>
#include <utility>

#include "../url.hpp"

namespace ncei {

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
	if (!impl_) {
		throw std::logic_error("Moved-from client");
	}
	return impl_->http;
}

const HttpClient& DataServiceClient::http_client() const {
	if (!impl_) {
		throw std::logic_error("Moved-from client");
	}
	return impl_->http;
}

Result<HttpResponse> DataServiceClient::do_get(std::string_view path) {
	if (!impl_) {
		return std::unexpected(Error::network("Client not initialized"));
	}
	return with_retry([&]() -> Result<HttpResponse> { return impl_->http.get(path); },
					  impl_->retry);
}

std::string DataServiceClient::build_data_query(const DataRequestParams& params) {
	std::string url = std::format("/data/v1?dataset={}", detail::encode(params.dataset));
	url.reserve(url.size() + 256);
	std::back_insert_iterator<std::string> out(url);

	if (params.start_date) {
		std::format_to(out, "&startDate={}", detail::encode(*params.start_date));
	}
	if (params.end_date) {
		std::format_to(out, "&endDate={}", detail::encode(*params.end_date));
	}
	if (params.stations) {
		bool first = true;
		for (const std::string& station : *params.stations) {
			std::format_to(out, "{}{}", first ? "&stations=" : ",", detail::encode(station));
			first = false;
		}
	}
	if (params.data_types) {
		bool first = true;
		for (const std::string& dt : *params.data_types) {
			std::format_to(out, "{}{}", first ? "&dataTypes=" : ",", detail::encode(dt));
			first = false;
		}
	}
	if (params.format)
		std::format_to(out, "&format={}", to_string(*params.format));
	if (params.units)
		std::format_to(out, "&units={}", detail::encode(*params.units));
	if (params.bbox)
		std::format_to(out, "&boundingBox={}", detail::encode(*params.bbox));
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

	switch (fmt) {
		case ResponseFormat::CSV:
			return parse_csv_data(response->body);
		case ResponseFormat::JSON: {
			DataPointCollection dpc;
			Result<void> ec = deserialize_data_point_collection(response->body, dpc);
			if (!ec) {
				return std::unexpected(ec.error());
			}
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
	std::string path = "/support/v3/datasets/" + detail::encode(dataset_id) + ".json";
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}

	DatasetMetadata meta;
	Result<void> ec = deserialize_dataset_metadata(response->body, meta);
	if (!ec) {
		return std::unexpected(ec.error());
	}
	return meta;
}

} // namespace ncei
