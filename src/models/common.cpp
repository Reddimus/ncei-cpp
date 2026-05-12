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

std::int32_t get_int(const glz::generic& obj, const char* key, std::int32_t def) {
	if (!obj.is_object()) {
		return def;
	}
	const glz::generic::object_t& o = obj.get_object();
	glz::generic::object_t::const_iterator it = o.find(key);
	if (it == o.end() || !it->second.is_number()) {
		return def;
	}
	return static_cast<std::int32_t>(it->second.get<double>());
}

void populate_result_set_metadata(const glz::generic& root, ResultSetMetadata& m) {
	if (!root.is_object()) {
		return;
	}
	const glz::generic::object_t& outer = root.get_object();
	glz::generic::object_t::const_iterator meta_it = outer.find("metadata");
	if (meta_it == outer.end() || !meta_it->second.is_object()) {
		return;
	}
	const glz::generic::object_t& meta = meta_it->second.get_object();
	glz::generic::object_t::const_iterator rs_it = meta.find("resultset");
	if (rs_it == meta.end() || !rs_it->second.is_object()) {
		return;
	}
	m.offset = get_int(rs_it->second, "offset", 0);
	m.count = get_int(rs_it->second, "count", 0);
	m.limit = get_int(rs_it->second, "limit", 25);
}

} // namespace detail

} // namespace ncei
