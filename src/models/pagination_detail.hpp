// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file pagination_detail.hpp
/// @brief Internal CDO list-response (`{metadata, results}` envelope) parser
///
/// NOT a public API. Lives under `src/models/` (NOT under `include/`) so it
/// is never installed. The CDO API wraps every list endpoint in this
/// envelope shape:
///
///   { "metadata": { "resultset": { "offset": N, "count": N, "limit": N } },
///     "results": [ { ...T... }, { ...T... }, ... ] }
///
/// Because every list endpoint shares the same envelope, we declare a
/// single templated `glz::meta<CDOResponse<T>>` here (paired with one for
/// `ResultSetMetadata` and a private `ResultSetWrapper` for the nested
/// `metadata.resultset` indirection) and Glaze handles the rest via its
/// reflected path. The per-T `glz::meta` definitions still live in the
/// per-model `.cpp` files; this header just stitches them into the
/// envelope.

#include "ncei/error.hpp"
#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <string_view>

#include "common_glaze_detail.hpp"

namespace ncei::detail {

/// The CDO API nests its pagination scalars one level deeper than our
/// `ResultSetMetadata` does (`metadata.resultset.{offset,count,limit}`).
/// We model that with a private envelope-shape struct, then read into a
/// CDOResponse<T> that includes this wrapper as its "metadata" field.
struct ResultSetWrapper {
	ResultSetMetadata resultset;
};

template <typename T>
struct CDOResponseEnvelope {
	ResultSetWrapper metadata;
	std::vector<T> results;
};

template <typename T>
[[nodiscard]] Result<void> deserialize_cdo_list_impl(std::string_view body, CDOResponse<T>& out) {
	CDOResponseEnvelope<T> envelope;
	glz::error_ctx ec = glz::read<kReadOpts>(envelope, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}
	out.metadata = envelope.metadata.resultset;
	out.results = std::move(envelope.results);
	return {};
}

} // namespace ncei::detail

// ===== Envelope metas =====
//
// Declared at namespace scope so Glaze's CPO finds them; the per-T
// `glz::meta<T>` definitions for the results-array element type live in
// the per-model `.cpp` files (Dataset, CDOStation, etc.).

template <>
struct glz::meta<ncei::ResultSetMetadata> {
	using T = ncei::ResultSetMetadata;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("offset", &T::offset, "count", &T::count, "limit", &T::limit);
};

template <>
struct glz::meta<ncei::detail::ResultSetWrapper> {
	using T = ncei::detail::ResultSetWrapper;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("resultset", &T::resultset);
};

template <typename ResultT>
struct glz::meta<ncei::detail::CDOResponseEnvelope<ResultT>> {
	using T = ncei::detail::CDOResponseEnvelope<ResultT>;
	static constexpr auto value = // auto-ok: glz::object returns unspellable tuple
		object("metadata", &T::metadata, "results", &T::results);
};
