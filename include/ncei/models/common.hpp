#pragma once

/// @file common.hpp
/// @brief Common Glaze-deserializer entry points for NCEI model types
///
/// Backed by [Glaze](https://github.com/stephenberry/glaze) for JSON
/// deserialization. The public surface from this header is the
/// `deserialize_*(std::string_view, T&) -> Result<void>` family. The
/// previous `from_json(const nlohmann::json&, T&)` overloads and the
/// transitional `json_string` / `json_int` / `json_double` / `json_bool`
/// helpers have been removed; downstream consumers (`crawler`,
/// `kalshi-trainer`) only use the high-level client methods, never
/// these internal helpers.

#include "ncei/error.hpp"
#include "ncei/models/cdo/data.hpp"
#include "ncei/models/cdo/data_category.hpp"
#include "ncei/models/cdo/data_type.hpp"
#include "ncei/models/cdo/dataset.hpp"
#include "ncei/models/cdo/location.hpp"
#include "ncei/models/cdo/location_category.hpp"
#include "ncei/models/cdo/station.hpp"
#include "ncei/models/data_service/data_point.hpp"
#include "ncei/models/data_service/dataset_metadata.hpp"
#include "ncei/models/data_service/search_result.hpp"
#include "ncei/pagination.hpp"

#include <string_view>
#include <vector>

namespace ncei {

// ===== Deserializers (Glaze-backed, return Result<void>) =====
//
// Each function parses a JSON body (string_view, zero-copy where possible)
// into the corresponding struct. On failure returns Error::parse(...).
//
// The CDO list-response family (parse a `{metadata, results}` envelope into
// the ResultSetMetadata + a vector<T>) is exposed via templated overloads
// in pagination.hpp; the single-record deserializers are below.

[[nodiscard]] Result<void> deserialize_dataset(std::string_view body, Dataset& out);
[[nodiscard]] Result<void> deserialize_data_category(std::string_view body, DataCategory& out);
[[nodiscard]] Result<void> deserialize_data_type(std::string_view body, DataType& out);
[[nodiscard]] Result<void> deserialize_location_category(std::string_view body,
														 LocationCategory& out);
[[nodiscard]] Result<void> deserialize_location(std::string_view body, Location& out);
[[nodiscard]] Result<void> deserialize_station(std::string_view body, CDOStation& out);
[[nodiscard]] Result<void> deserialize_data_record(std::string_view body, DataRecord& out);

[[nodiscard]] Result<void> deserialize_data_point_collection(std::string_view body,
															 DataPointCollection& out);
[[nodiscard]] Result<void> deserialize_dataset_metadata(std::string_view body,
														DatasetMetadata& out);
[[nodiscard]] Result<void> deserialize_data_search_result(std::string_view body,
														  DataSearchResult& out);
[[nodiscard]] Result<void> deserialize_dataset_search_result(std::string_view body,
															 DatasetSearchResult& out);

// CDO list-response (envelope { metadata: {...}, results: [...] }) deserializer
// — templated, defined in pagination.hpp where CDOResponse<T> lives.
template <typename T>
[[nodiscard]] Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<T>& out);

} // namespace ncei
