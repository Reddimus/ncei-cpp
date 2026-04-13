#pragma once
#include "ncei/error.hpp"
#include "ncei/format.hpp"
#include "ncei/http_client.hpp"
#include "ncei/models/data_service/data_point.hpp"
#include "ncei/models/data_service/dataset_metadata.hpp"
#include "ncei/models/data_service/search_result.hpp"
#include "ncei/retry.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ncei {

struct DataRequestParams {
	std::string dataset; // REQUIRED: e.g. "daily-summaries"
	std::optional<std::string> start_date;
	std::optional<std::string> end_date;
	std::optional<std::vector<std::string>> stations;
	std::optional<std::vector<std::string>> data_types;
	std::optional<ResponseFormat> format;
	std::optional<std::string> units;
	std::optional<std::string> bbox;
	std::optional<bool> include_attributes;
	std::optional<bool> include_station_name;
	std::optional<bool> include_station_location;
};

struct DataSearchParams {
	std::string dataset;
	std::optional<std::string> start_date;
	std::optional<std::string> end_date;
	std::optional<std::vector<std::string>> stations;
	std::optional<std::vector<std::string>> data_types;
	std::optional<std::string> bbox;
	std::optional<std::string> text;
};

class DataServiceClient {
public:
	struct Config {
		ClientConfig http;
		RetryPolicy retry{};
	};

	explicit DataServiceClient(Config config);
	~DataServiceClient();
	DataServiceClient(DataServiceClient&&) noexcept;
	DataServiceClient& operator=(DataServiceClient&&) noexcept;
	DataServiceClient(const DataServiceClient&) = delete;
	DataServiceClient& operator=(const DataServiceClient&) = delete;

	[[nodiscard]] Result<DataPointCollection> get_data(const DataRequestParams& params);
	[[nodiscard]] Result<std::string> get_data_raw(const DataRequestParams& params);
	[[nodiscard]] Result<DatasetMetadata> get_dataset_metadata(const std::string& dataset_id);

	[[nodiscard]] HttpClient& http_client();
	[[nodiscard]] const HttpClient& http_client() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
	[[nodiscard]] Result<HttpResponse> do_get(std::string_view path);
	[[nodiscard]] std::string build_data_query(const DataRequestParams& params);
};

} // namespace ncei
