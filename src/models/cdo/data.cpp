// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/cdo/data.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../pagination_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::DataRecord> {
	using T = ncei::DataRecord;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("date", &T::date, "datatype", &T::datatype, "station", &T::station, "attributes",
			   &T::attributes, "value", &T::value);
};

namespace ncei {

Result<void> deserialize_data_record(std::string_view body, DataRecord& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

template <>
Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<DataRecord>& out) {
	return detail::deserialize_cdo_list_impl(body, out);
}

} // namespace ncei
