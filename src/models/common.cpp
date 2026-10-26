// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/common.hpp"

#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "common_glaze_detail.hpp"

namespace ncei {

// ===== Glaze detail helpers (internal) =====
//
// Pulled out of every `.cpp` so the null-safety contract lives in one
// place. The pre-migration nlohmann path returned defaults for null /
// missing fields; we preserve that here so the CDO API's frequent
// `"datacoverage": null` rows don't blow up the parser.

namespace detail {

std::string get_string(const glz::generic& obj, const char* key) {
	if (!obj.is_object()) {
		return {};
	}
	const glz::generic::object_t& o = obj.get_object();
	glz::generic::object_t::const_iterator it = o.find(key);
	if (it == o.end() || !it->second.is_string()) {
		return {};
	}
	return it->second.get<std::string>();
}

double get_double(const glz::generic& obj, const char* key, double def) {
	if (!obj.is_object()) {
		return def;
	}
	const glz::generic::object_t& o = obj.get_object();
	glz::generic::object_t::const_iterator it = o.find(key);
	if (it == o.end() || !it->second.is_number()) {
		return def;
	}
	return it->second.get<double>();
}

} // namespace detail

} // namespace ncei
