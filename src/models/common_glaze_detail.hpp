// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file common_glaze_detail.hpp
/// @brief Internal Glaze helpers shared across model translation units
///
/// NOT a public API. Lives under `src/models/` (NOT under `include/`) so it
/// is never installed — downstream consumers (crawler, kalshi-trainer)
/// cannot accidentally depend on these symbols. They exist only so the
/// per-model `.cpp` files and the CDO list-response template in
/// `pagination_detail.hpp` can share the dynamic-key dispatch helpers
/// without duplicating them.

#include "ncei/pagination.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>

namespace ncei::detail {

/// Custom Glaze opts struct: extends `glz::opts` with the optional
/// `skip_null_members_on_read` flag. Standard `glz::opts` doesn't include
/// that field; setting it requires a derived struct (the Glaze v7.6.0
/// pattern used internally by `glz::set_opt<&opts::skip_null_members_on_read>`,
/// just spelled out explicitly here so the call sites can stay flat).
///
/// The combined config:
///   - `error_on_unknown_keys = false`  → CDO frequently adds new metadata
///     keys that older client builds don't model; we must keep parsing.
///   - `skip_null_members_on_read = true` → CDO routinely emits
///     `"datacoverage": null`, `"elevation": null`, etc. Without this,
///     Glaze errors on null-for-scalar; with it, the field is left at
///     its in-struct default (matches the pre-migration `json_double()`
///     helper semantics).
struct ncei_opts : glz::opts {
	bool skip_null_members_on_read = true;
};

inline constexpr ncei_opts kReadOpts{{.error_on_unknown_keys = false}, true};

/// Null-safe string extraction from a glz::generic object. Returns "" if
/// the key is missing or the value is null/non-string. Mirrors the
/// pre-migration `json_string()` helper.
[[nodiscard]] std::string get_string(const glz::generic& obj, const char* key);

/// Null-safe double extraction. Returns `def` if missing or null/non-number.
[[nodiscard]] double get_double(const glz::generic& obj, const char* key, double def = 0.0);

/// Null-safe int32 extraction. Returns `def` if missing or null/non-number.
[[nodiscard]] std::int32_t get_int(const glz::generic& obj, const char* key, std::int32_t def = 0);

/// Populate the `{offset, count, limit}` fields of a ResultSetMetadata
/// from the `metadata.resultset` sub-object of a glz::generic root. If
/// either the outer or inner key is missing or not an object, leaves
/// `m` at its default values.
void populate_result_set_metadata(const glz::generic& root, ResultSetMetadata& m);

} // namespace ncei::detail
