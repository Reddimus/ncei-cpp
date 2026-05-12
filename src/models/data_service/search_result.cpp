// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/data_service/search_result.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../common_glaze_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::DataSearchResult> {
	using T = ncei::DataSearchResult;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::station_id, "name", &T::station_name, "latitude", &T::latitude,
			   "longitude", &T::longitude, "elevation", &T::elevation, "mindate", &T::min_date,
			   "maxdate", &T::max_date, "datacoverage", &T::data_coverage);
};

template <>
struct glz::meta<ncei::DatasetSearchResult> {
	using T = ncei::DatasetSearchResult;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("uid", &T::uid, "name", &T::name, "description", &T::description, "startDate",
			   &T::start_date, "endDate", &T::end_date, "dataTypes", &T::data_types);
};

namespace ncei {

Result<void> deserialize_data_search_result(std::string_view body, DataSearchResult& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

Result<void> deserialize_dataset_search_result(std::string_view body, DatasetSearchResult& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

} // namespace ncei
