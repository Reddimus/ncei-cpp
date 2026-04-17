#include "ncei/cdo_client.hpp"

#include "ncei/models/common.hpp"

#include <format>
#include <iterator>
#include <nlohmann/json.hpp>
#include <utility>

namespace ncei {

struct CDOClient::Impl {
	HttpClient http;
	RateLimiter rate_limiter;
	RetryPolicy retry;

	Impl(ClientConfig http_config, RateLimiter::Config rl_config, RetryPolicy rp)
		: http(std::move(http_config)), rate_limiter(rl_config), retry(std::move(rp)) {}
};

CDOClient::CDOClient(Config config) {
	if (config.http.base_url.empty()) {
		config.http.base_url = "https://www.ncei.noaa.gov/cdo-web/api/v2";
	}
	config.http.extra_headers.emplace_back("token", config.token);

	impl_ = std::make_unique<Impl>(std::move(config.http), config.rate_limit, config.retry);
}

CDOClient::~CDOClient() = default;
CDOClient::CDOClient(CDOClient&&) noexcept = default;
CDOClient& CDOClient::operator=(CDOClient&&) noexcept = default;

HttpClient& CDOClient::http_client() {
	return impl_->http;
}

const HttpClient& CDOClient::http_client() const {
	return impl_->http;
}

std::int32_t CDOClient::daily_requests_remaining() const noexcept {
	return impl_->rate_limiter.daily_requests_remaining();
}

Result<HttpResponse> CDOClient::do_get(std::string_view path) {
	if (impl_->rate_limiter.daily_requests_remaining() <= 0) {
		return std::unexpected(Error::quota_exceeded("CDO daily request quota exceeded"));
	}

	if (!impl_->rate_limiter.acquire()) {
		return std::unexpected(Error::rate_limited("Rate limit acquisition timed out"));
	}

	return with_retry([&]() -> Result<HttpResponse> { return impl_->http.get(path); },
					  impl_->retry);
}

std::string CDOClient::build_list_query(std::string_view base, const CDOListParams& params) {
	std::string url(base);
	url.reserve(url.size() + 256);
	std::back_insert_iterator<std::string> out(url);
	char sep = '?';

	auto append = [&](std::string_view key, const auto& value) {
		std::format_to(out, "{}{}={}", sep, key, value);
		sep = '&';
	};

	if (params.dataset_id) append("datasetid", *params.dataset_id);
	if (params.location_id) append("locationid", *params.location_id);
	if (params.station_id) append("stationid", *params.station_id);
	if (params.data_type_id) append("datatypeid", *params.data_type_id);
	if (params.data_category_id) append("datacategoryid", *params.data_category_id);
	if (params.location_category_id) append("locationcategoryid", *params.location_category_id);
	if (params.start_date) append("startdate", *params.start_date);
	if (params.end_date) append("enddate", *params.end_date);
	if (params.sort_field) append("sortfield", *params.sort_field);
	if (params.sort_order) append("sortorder", *params.sort_order);
	if (params.limit) append("limit", *params.limit);
	if (params.offset) append("offset", *params.offset);

	return url;
}

std::string CDOClient::build_data_query(const GetDataParams& params) {
	std::string url = std::format("/data?datasetid={}&startdate={}&enddate={}",
								  params.dataset_id, params.start_date, params.end_date);
	std::back_insert_iterator<std::string> out(url);

	if (params.data_type_ids) {
		for (const std::string& dt : *params.data_type_ids) {
			std::format_to(out, "&datatypeid={}", dt);
		}
	}
	if (params.location_id) std::format_to(out, "&locationid={}", *params.location_id);
	if (params.station_id) std::format_to(out, "&stationid={}", *params.station_id);
	if (params.sort_field) std::format_to(out, "&sortfield={}", *params.sort_field);
	if (params.sort_order) std::format_to(out, "&sortorder={}", *params.sort_order);
	if (params.limit) std::format_to(out, "&limit={}", *params.limit);
	if (params.offset) std::format_to(out, "&offset={}", *params.offset);
	if (params.units) std::format_to(out, "&units={}", *params.units);
	if (params.include_metadata) {
		std::format_to(out, "&includemetadata={}",
					   *params.include_metadata ? "true" : "false");
	}

	return url;
}

namespace {

template <typename T>
Result<CDOResponse<T>> parse_list_response(const HttpResponse& response) {
	try {
		nlohmann::json j = nlohmann::json::parse(response.body);
		CDOResponse<T> result;

		// Parse metadata envelope
		ResultSetMetadata meta;
		from_json(j, meta);
		result.metadata = meta;

		// Parse results array
		if (j.contains("results") && j["results"].is_array()) {
			result.results.reserve(j["results"].size());
			for (const nlohmann::json& item : j["results"]) {
				T obj;
				from_json(item, obj);
				result.results.push_back(std::move(obj));
			}
		}

		return result;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(std::string("JSON parse error: ") + e.what()));
	}
}

template <typename T>
Result<T> parse_single_response(const HttpResponse& response) {
	try {
		nlohmann::json j = nlohmann::json::parse(response.body);
		T obj;
		from_json(j, obj);
		return obj;
	} catch (const nlohmann::json::exception& e) {
		return std::unexpected(Error::parse(std::string("JSON parse error: ") + e.what()));
	}
}

} // namespace

// --- Datasets ---

Result<CDOResponse<Dataset>> CDOClient::get_datasets(const CDOListParams& params) {
	std::string query = build_list_query("/datasets", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<Dataset>(*response);
}

Result<Dataset> CDOClient::get_dataset(const std::string& id) {
	std::string path = "/datasets/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<Dataset>(*response);
}

// --- Data Categories ---

Result<CDOResponse<DataCategory>> CDOClient::get_data_categories(const CDOListParams& params) {
	std::string query = build_list_query("/datacategories", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<DataCategory>(*response);
}

Result<DataCategory> CDOClient::get_data_category(const std::string& id) {
	std::string path = "/datacategories/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<DataCategory>(*response);
}

// --- Data Types ---

Result<CDOResponse<DataType>> CDOClient::get_data_types(const CDOListParams& params) {
	std::string query = build_list_query("/datatypes", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<DataType>(*response);
}

Result<DataType> CDOClient::get_data_type(const std::string& id) {
	std::string path = "/datatypes/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<DataType>(*response);
}

// --- Location Categories ---

Result<CDOResponse<LocationCategory>>
CDOClient::get_location_categories(const CDOListParams& params) {
	std::string query = build_list_query("/locationcategories", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<LocationCategory>(*response);
}

Result<LocationCategory> CDOClient::get_location_category(const std::string& id) {
	std::string path = "/locationcategories/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<LocationCategory>(*response);
}

// --- Locations ---

Result<CDOResponse<Location>> CDOClient::get_locations(const CDOListParams& params) {
	std::string query = build_list_query("/locations", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<Location>(*response);
}

Result<Location> CDOClient::get_location(const std::string& id) {
	std::string path = "/locations/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<Location>(*response);
}

// --- Stations ---

Result<CDOResponse<CDOStation>> CDOClient::get_stations(const CDOListParams& params) {
	std::string query = build_list_query("/stations", params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<CDOStation>(*response);
}

Result<CDOStation> CDOClient::get_station(const std::string& id) {
	std::string path = "/stations/" + id;
	Result<HttpResponse> response = do_get(path);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_single_response<CDOStation>(*response);
}

// --- Data ---

Result<CDOResponse<DataRecord>> CDOClient::get_data(const GetDataParams& params) {
	std::string query = build_data_query(params);
	Result<HttpResponse> response = do_get(query);
	if (!response) {
		return std::unexpected(response.error());
	}
	if (response->status_code != 200) {
		return std::unexpected(Error::from_response(response->status_code, response->body));
	}
	return parse_list_response<DataRecord>(*response);
}

Result<std::vector<DataRecord>> CDOClient::get_data_all(const GetDataParams& params) {
	std::vector<DataRecord> all_records;
	std::vector<DateRange> ranges = split_date_range(params.start_date, params.end_date);

	for (const DateRange& range : ranges) {
		std::int32_t current_offset = 0;
		bool has_more = true;

		while (has_more) {
			GetDataParams chunk_params = params;
			chunk_params.start_date = range.first;
			chunk_params.end_date = range.second;
			chunk_params.offset = current_offset;
			if (!chunk_params.limit) {
				chunk_params.limit = 1000; // CDO max per request
			}

			Result<CDOResponse<DataRecord>> result = get_data(chunk_params);
			if (!result) {
				return std::unexpected(result.error());
			}

			for (DataRecord& record : result->results) {
				all_records.push_back(std::move(record));
			}

			has_more = result->has_more();
			if (has_more) {
				current_offset = result->next_offset();
			}
		}
	}

	return all_records;
}

} // namespace ncei
