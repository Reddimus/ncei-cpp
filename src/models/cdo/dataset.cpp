// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/cdo/dataset.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../pagination_detail.hpp"

namespace ncei {

// Dataset is fully-statically-typed. Glaze's direct reflected path
// handles it via the meta below; null values for any field (CDO's
// `"datacoverage": null`, etc.) are tolerated via
// skip_null_members_on_read, which leaves the field at its in-struct
// default.

} // namespace ncei

template <>
struct glz::meta<ncei::Dataset> {
	using T = ncei::Dataset;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name, "datacoverage", &T::data_coverage, "mindate",
			   &T::min_date, "maxdate", &T::max_date);
};

namespace ncei {

Result<void> deserialize_dataset(std::string_view body, Dataset& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

template <>
Result<void> deserialize_cdo_list(std::string_view body, CDOResponse<Dataset>& out) {
	return detail::deserialize_cdo_list_impl(body, out);
}

} // namespace ncei
