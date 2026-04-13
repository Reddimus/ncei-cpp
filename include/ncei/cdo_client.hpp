#pragma once

#include "ncei/date_range.hpp"
#include "ncei/error.hpp"
#include "ncei/http_client.hpp"
#include "ncei/models/cdo/data.hpp"
#include "ncei/models/cdo/data_category.hpp"
#include "ncei/models/cdo/data_type.hpp"
#include "ncei/models/cdo/dataset.hpp"
#include "ncei/models/cdo/location.hpp"
#include "ncei/models/cdo/location_category.hpp"
#include "ncei/models/cdo/station.hpp"
#include "ncei/pagination.hpp"
#include "ncei/rate_limit.hpp"
#include "ncei/retry.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ncei {

struct CDOListParams {
	std::optional<std::string> dataset_id;
	std::optional<std::string> location_id;
	std::optional<std::string> station_id;
	std::optional<std::string> data_type_id;
	std::optional<std::string> data_category_id;
	std::optional<std::string> location_category_id;
	std::optional<std::string> start_date;
	std::optional<std::string> end_date;
	std::optional<std::string> sort_field;
	std::optional<std::string> sort_order;
	std::optional<std::int32_t> limit;
	std::optional<std::int32_t> offset;
};

struct GetDataParams {
	std::string dataset_id; // REQUIRED
	std::string start_date; // REQUIRED
	std::string end_date;	// REQUIRED
	std::optional<std::vector<std::string>> data_type_ids;
	std::optional<std::string> location_id;
	std::optional<std::string> station_id;
	std::optional<std::string> sort_field;
	std::optional<std::string> sort_order;
	std::optional<std::int32_t> limit;
	std::optional<std::int32_t> offset;
	std::optional<std::string> units;
	std::optional<bool> include_metadata;
};

class CDOClient {
public:
	struct Config {
		std::string token; // REQUIRED
		ClientConfig http;
		RateLimiter::Config rate_limit{};
		RetryPolicy retry{};
	};

	explicit CDOClient(Config config);
	~CDOClient();
	CDOClient(CDOClient&&) noexcept;
	CDOClient& operator=(CDOClient&&) noexcept;
	CDOClient(const CDOClient&) = delete;
	CDOClient& operator=(const CDOClient&) = delete;

	// Datasets
	[[nodiscard]] Result<CDOResponse<Dataset>> get_datasets(const CDOListParams& params = {});
	[[nodiscard]] Result<Dataset> get_dataset(const std::string& id);

	// Data Categories
	[[nodiscard]] Result<CDOResponse<DataCategory>>
	get_data_categories(const CDOListParams& params = {});
	[[nodiscard]] Result<DataCategory> get_data_category(const std::string& id);

	// Data Types
	[[nodiscard]] Result<CDOResponse<DataType>> get_data_types(const CDOListParams& params = {});
	[[nodiscard]] Result<DataType> get_data_type(const std::string& id);

	// Location Categories
	[[nodiscard]] Result<CDOResponse<LocationCategory>>
	get_location_categories(const CDOListParams& params = {});
	[[nodiscard]] Result<LocationCategory> get_location_category(const std::string& id);

	// Locations
	[[nodiscard]] Result<CDOResponse<Location>> get_locations(const CDOListParams& params = {});
	[[nodiscard]] Result<Location> get_location(const std::string& id);

	// Stations
	[[nodiscard]] Result<CDOResponse<CDOStation>> get_stations(const CDOListParams& params = {});
	[[nodiscard]] Result<CDOStation> get_station(const std::string& id);

	// Data
	[[nodiscard]] Result<CDOResponse<DataRecord>> get_data(const GetDataParams& params);
	[[nodiscard]] Result<std::vector<DataRecord>> get_data_all(const GetDataParams& params);

	[[nodiscard]] HttpClient& http_client();
	[[nodiscard]] const HttpClient& http_client() const;
	[[nodiscard]] std::int32_t daily_requests_remaining() const noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
	[[nodiscard]] Result<HttpResponse> do_get(std::string_view path);
	[[nodiscard]] std::string build_list_query(std::string_view base, const CDOListParams& params);
	[[nodiscard]] std::string build_data_query(const GetDataParams& params);
};

} // namespace ncei
