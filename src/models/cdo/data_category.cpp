// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/cdo/data_category.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../pagination_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::DataCategory> {
	using T = ncei::DataCategory;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name);
};

namespace ncei {

Result<void> deserialize_data_category(std::string_view body, DataCategory& out) {
	DataCategory parsed{};
	glz::error_ctx ec = glz::read<detail::kReadOpts>(parsed, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	out = std::move(parsed);
	return {};
}

template <>
Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<DataCategory>& out) {
	return detail::deserialize_cdo_list_impl(body, out);
}

} // namespace ncei
