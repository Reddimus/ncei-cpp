// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/data_service/data_point.hpp"

#include "ncei/csv_parser.hpp"
#include "ncei/error.hpp"
#include "ncei/models/common.hpp"

#include <algorithm>
#include <exception>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "../common_glaze_detail.hpp"

namespace ncei {

namespace {

const std::set<std::string, std::less<>> kKnownFields = {"DATE",	 "STATION",	  "NAME",
														 "LATITUDE", "LONGITUDE", "ELEVATION"};

void populate_from_row(DataPoint& dp, const std::vector<std::string>& headers,
					   const std::vector<std::string>& values) {
	std::size_t count = std::min(headers.size(), values.size());
	for (std::size_t i = 0; i < count; ++i) {
		const std::string& col = headers[i];
		const std::string& val = values[i];
		if (col == "DATE") {
			dp.date = val;
		} else if (col == "STATION") {
			dp.station = val;
		} else if (col == "NAME") {
			dp.name = val;
		} else if (col == "LATITUDE") {
			try {
				dp.latitude = std::stod(val);
			} catch (const std::exception&) {
				dp.latitude = 0.0;
			}
		} else if (col == "LONGITUDE") {
			try {
				dp.longitude = std::stod(val);
			} catch (const std::exception&) {
				dp.longitude = 0.0;
			}
		} else if (col == "ELEVATION") {
			try {
				dp.elevation = std::stod(val);
			} catch (const std::exception&) {
				dp.elevation = 0.0;
			}
		} else {
			dp.attributes.emplace_back(col, val);
		}
	}
}

DataPointCollection parse_delimited(std::string_view text, DelimitedParser::Delimiter delim) {
	DataPointCollection collection;
	DelimitedParser parser(delim);
	std::vector<std::vector<std::string>> rows = parser.parse(text);

	if (rows.empty()) {
		return collection;
	}

	collection.columns = rows[0];

	collection.records.reserve(rows.size() - 1);
	for (std::size_t i = 1; i < rows.size(); ++i) {
		DataPoint dp;
		populate_from_row(dp, collection.columns, rows[i]);
		collection.records.push_back(std::move(dp));
	}

	return collection;
}

// ===== Glaze AST walkers =====
//
// TODO(glaze): DataPoint has DYNAMIC KEYS (user-requested data types like
// TMAX / TMIN / PRCP / SNOW are tacked on as siblings to the known
// DATE/STATION/... fields), so we cannot pre-declare a glz::meta. We parse
// the response into a glz::generic AST once and walk it. The same applies
// to DataPointCollection because the per-record shape varies between
// queries.

void populate_data_point_from_object(const glz::generic& root, DataPoint& dp) {
	if (!root.is_object()) {
		return;
	}
	dp.date = detail::get_string(root, "DATE");
	dp.station = detail::get_string(root, "STATION");
	dp.name = detail::get_string(root, "NAME");
	dp.latitude = detail::get_double(root, "LATITUDE");
	dp.longitude = detail::get_double(root, "LONGITUDE");
	dp.elevation = detail::get_double(root, "ELEVATION");

	dp.attributes.clear();
	for (const auto& [key, val] : root.get_object()) {
		if (kKnownFields.find(key) != kKnownFields.end()) {
			continue;
		}
		std::string value;
		if (val.is_string()) {
			value = val.get<std::string>();
		} else if (val.is_number()) {
			value = std::to_string(val.get<double>());
		} else if (val.is_null()) {
			value.clear();
		} else {
			// Re-serialize anything else (booleans, arrays, nested objects)
			// to preserve information without forcing a typed schema.
			glz::error_ctx ec = glz::write_json(val, value);
			if (ec) {
				value.clear();
			}
		}
		dp.attributes.emplace_back(key, std::move(value));
	}
}

void collect_columns(const glz::generic& obj, std::vector<std::string>& columns) {
	if (!obj.is_object()) {
		return;
	}
	for (const auto& [key, _] : obj.get_object()) {
		columns.push_back(key);
	}
}

} // namespace

std::optional<std::string> DataPoint::get(std::string_view key) const {
	for (const std::pair<std::string, std::string>& attr : attributes) {
		if (attr.first == key) {
			return attr.second;
		}
	}
	return std::nullopt;
}

std::optional<double> DataPoint::get_double(std::string_view key) const {
	std::optional<std::string> val = get(key);
	if (!val) {
		return std::nullopt;
	}
	try {
		return std::stod(*val);
	} catch (const std::exception&) {
		return std::nullopt;
	}
}

Result<void> deserialize_data_point_collection(std::string_view body, DataPointCollection& out) {
	out.columns.clear();
	out.records.clear();

	glz::generic root{};
	glz::error_ctx ec = glz::read_json(root, body);
	if (ec) {
		return std::unexpected(Error::parse(glz::format_error(ec, body)));
	}

	if (root.is_array()) {
		const glz::generic::array_t& arr = root.get_array();
		if (!arr.empty()) {
			collect_columns(arr.front(), out.columns);
		}
		out.records.reserve(arr.size());
		for (const glz::generic& item : arr) {
			DataPoint dp;
			populate_data_point_from_object(item, dp);
			out.records.push_back(std::move(dp));
		}
	} else if (root.is_object()) {
		collect_columns(root, out.columns);
		DataPoint dp;
		populate_data_point_from_object(root, dp);
		out.records.push_back(std::move(dp));
	}

	return {};
}

DataPointCollection parse_csv_data(std::string_view csv_text) {
	return parse_delimited(csv_text, DelimitedParser::Delimiter::Comma);
}

DataPointCollection parse_ssv_data(std::string_view ssv_text) {
	return parse_delimited(ssv_text, DelimitedParser::Delimiter::Space);
}

} // namespace ncei
