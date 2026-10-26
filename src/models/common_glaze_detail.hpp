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

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>

#include "../json.hpp"

namespace ncei::detail {

/// Null-safe string extraction from a glz::generic object. Returns "" if
/// the key is missing or the value is null/non-string. Mirrors the
/// pre-migration `json_string()` helper.
[[nodiscard]] std::string get_string(const glz::generic& obj, const char* key);

/// Null-safe double extraction. Returns `def` if missing or null/non-number.
[[nodiscard]] double get_double(const glz::generic& obj, const char* key, double def = 0.0);

} // namespace ncei::detail
