// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/data_service/dataset_metadata.hpp"

#include "ncei/error.hpp"
#include "ncei/models/common.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "../common_glaze_detail.hpp"

namespace ncei {} // namespace ncei

template <>
struct glz::meta<ncei::DatasetField> {
	using T = ncei::DatasetField;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name, "description", &T::description, "dataType",
			   &T::data_type);
};

template <>
struct glz::meta<ncei::DatasetMetadata> {
	using T = ncei::DatasetMetadata;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("id", &T::id, "name", &T::name, "description", &T::description, "fields",
			   &T::fields);
};

namespace ncei {

Result<void> deserialize_dataset_metadata(std::string_view body, DatasetMetadata& out) {
	glz::error_ctx ec = glz::read<detail::kReadOpts>(out, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	return {};
}

} // namespace ncei
