// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/cdo/location_category.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../pagination_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::LocationCategory> {
	using T = ncei::LocationCategory;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name);
};

namespace ncei {

Result<void> deserialize_location_category(std::string_view body, LocationCategory& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

template <>
Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<LocationCategory>& out) {
	return detail::deserialize_cdo_list_impl(body, out);
}

} // namespace ncei
