// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/cdo/data_type.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../pagination_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::DataType> {
	using T = ncei::DataType;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name, "datacoverage", &T::data_coverage, "mindate",
			   &T::min_date, "maxdate", &T::max_date);
};

namespace ncei {

Result<void> deserialize_data_type(std::string_view body, DataType& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

template <>
Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<DataType>& out) {
	return detail::deserialize_cdo_list_impl(body, out);
}

} // namespace ncei
