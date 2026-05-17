// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/models/climate_index.hpp"

#include "ncei/csv_parser.hpp"

#include <cctype>
#include <cstdlib>
#include <glaze/glaze.hpp>
#include <glaze/json/generic.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "common_glaze_detail.hpp"

namespace ncei {

namespace {

std::string trim(std::string_view s) {
	std::size_t a = s.find_first_not_of(" \t\r\n");
	if (a == std::string_view::npos) {
		return {};
	}
	std::size_t b = s.find_last_not_of(" \t\r\n");
	return std::string(s.substr(a, b - a + 1));
}

// Tolerant double parse: handles GISTEMP's leading-dot form ("-.19"),
// missing markers ("***", "", "-9999", "-99.99"). Returns false if not a
// usable value.
bool parse_value(std::string_view raw, double& out) {
	const std::string t = trim(raw);
	if (t.empty() || t == "***" || t == "-9999" || t == "-99.99" || t == "NA") {
		return false;
	}
	const char* b = t.c_str();
	char* end = nullptr;
	const double v = std::strtod(b, &end);
	if (end == b) {
		return false;
	}
	out = v;
	return true;
}

int header_index(const std::vector<std::string>& header, std::string_view name) {
	for (std::size_t i = 0; i < header.size(); ++i) {
		if (trim(header[i]) == name) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

} // namespace

Result<void> deserialize_cag_series(std::string_view body, CagSeries& out) {
	glz::generic root{};
	const glz::error_ctx ec = glz::read_json(root, std::string(body));
	if (ec || !root.is_object()) {
		return std::unexpected(Error::parse("CAG: invalid JSON"));
	}
	const glz::generic::object_t& o = root.get_object();

	glz::generic::object_t::const_iterator dit = o.find("description");
	if (dit != o.end() && dit->second.is_object()) {
		out.title = detail::get_string(dit->second, "title");
		out.units = detail::get_string(dit->second, "units");
		out.base_period = detail::get_string(dit->second, "base_period");
	}

	glz::generic::object_t::const_iterator data = o.find("data");
	if (data == o.end() || !data->second.is_object()) {
		return std::unexpected(Error::parse("CAG: no data object"));
	}
	for (const std::pair<const std::string, glz::generic>& kv : data->second.get_object()) {
		double v = 0.0;
		bool ok = false;
		if (kv.second.is_string()) {
			ok = parse_value(kv.second.get<std::string>(), v);
		} else if (kv.second.is_number()) {
			v = kv.second.get<double>();
			ok = true;
		}
		if (ok) {
			out.data.push_back(ClimatePoint{kv.first, v});
		}
	}
	if (out.data.empty()) {
		return std::unexpected(Error::parse("CAG: empty data series"));
	}
	return {};
}

Result<void> deserialize_gistemp_csv(std::string_view body, GistempSeries& out) {
	const DelimitedParser parser(DelimitedParser::Delimiter::Comma);
	const std::vector<std::vector<std::string>> rows = parser.parse(body);

	int year_col = -1;
	int jd_col = -1;
	bool in_data = false;
	for (const std::vector<std::string>& row : rows) {
		if (row.empty()) {
			continue;
		}
		if (!in_data) {
			const int yc = header_index(row, "Year");
			const int jc = header_index(row, "J-D");
			if (yc >= 0 && jc >= 0) {
				year_col = yc;
				jd_col = jc;
				in_data = true;
			}
			continue;
		}
		if (static_cast<int>(row.size()) <= jd_col) {
			continue;
		}
		const std::string yr = trim(row[static_cast<std::size_t>(year_col)]);
		if (yr.empty() || !std::isdigit(static_cast<unsigned char>(yr[0]))) {
			continue; // repeated header / banner rows
		}
		double v = 0.0;
		if (parse_value(row[static_cast<std::size_t>(jd_col)], v)) {
			out.annual.push_back(ClimatePoint{yr, v});
		}
	}
	if (out.annual.empty()) {
		return std::unexpected(Error::parse("GISTEMP: no annual J-D values"));
	}
	return {};
}

Result<void> deserialize_nsidc_sea_ice_csv(std::string_view body, bool monthly, SeaIceSeries& out) {
	out.monthly = monthly;
	const DelimitedParser parser(DelimitedParser::Delimiter::Comma);
	const std::vector<std::vector<std::string>> rows = parser.parse(body);
	if (rows.empty()) {
		return std::unexpected(Error::parse("NSIDC: empty CSV"));
	}

	const std::vector<std::string>& header = rows.front();
	const int yi = header_index(header, monthly ? "year" : "Year");
	const int ext = header_index(header, monthly ? "extent" : "Extent");
	const int mi = header_index(header, monthly ? "mo" : "Month");
	const int di = header_index(header, "Day");
	if (yi < 0 || ext < 0) {
		return std::unexpected(Error::parse("NSIDC: unrecognized CSV header"));
	}

	for (std::size_t r = 1; r < rows.size(); ++r) {
		const std::vector<std::string>& row = rows[r];
		if (static_cast<int>(row.size()) <= ext) {
			continue;
		}
		double extent = 0.0;
		if (!parse_value(row[static_cast<std::size_t>(ext)], extent) || extent <= 0.0) {
			continue;
		}
		const std::string y = trim(row[static_cast<std::size_t>(yi)]);
		if (y.empty()) {
			continue;
		}
		std::string period = y;
		if (mi >= 0 && static_cast<int>(row.size()) > mi) {
			const std::string m = trim(row[static_cast<std::size_t>(mi)]);
			if (m.size() == 1) {
				period += "-0" + m;
			} else if (!m.empty()) {
				period += "-" + m;
			}
		}
		if (!monthly && di >= 0 && static_cast<int>(row.size()) > di) {
			const std::string d = trim(row[static_cast<std::size_t>(di)]);
			if (d.size() == 1) {
				period += "-0" + d;
			} else if (!d.empty()) {
				period += "-" + d;
			}
		}
		out.extent.push_back(ClimatePoint{period, extent});
	}
	if (out.extent.empty()) {
		return std::unexpected(Error::parse("NSIDC: no usable extent rows"));
	}
	return {};
}

} // namespace ncei
