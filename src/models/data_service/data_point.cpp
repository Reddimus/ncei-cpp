#include "ncei/models/data_service/data_point.hpp"

#include "ncei/csv_parser.hpp"
#include "ncei/models/common.hpp"

#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string>

namespace ncei {

namespace {

const std::set<std::string> known_fields = {"DATE",		"STATION",	 "NAME",
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

void from_json(const nlohmann::json& j, DataPoint& dp) {
	dp.date = json_string(j, "DATE");
	dp.station = json_string(j, "STATION");
	dp.name = json_string(j, "NAME");
	dp.latitude = json_double(j, "LATITUDE");
	dp.longitude = json_double(j, "LONGITUDE");
	dp.elevation = json_double(j, "ELEVATION");

	dp.attributes.clear();
	for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it) {
		if (known_fields.find(it.key()) == known_fields.end()) {
			std::string value;
			if (it.value().is_string()) {
				value = it.value().get<std::string>();
			} else if (it.value().is_number()) {
				value = std::to_string(it.value().get<double>());
			} else if (!it.value().is_null()) {
				value = it.value().dump();
			}
			dp.attributes.emplace_back(it.key(), std::move(value));
		}
	}
}

void from_json(const nlohmann::json& j, DataPointCollection& dpc) {
	dpc.columns.clear();
	dpc.records.clear();

	if (j.is_array()) {
		if (!j.empty()) {
			// Extract column names from first element's keys
			for (nlohmann::json::const_iterator it = j[0].begin(); it != j[0].end(); ++it) {
				dpc.columns.push_back(it.key());
			}
		}
		dpc.records.reserve(j.size());
		for (const nlohmann::json& item : j) {
			DataPoint dp;
			from_json(item, dp);
			dpc.records.push_back(std::move(dp));
		}
	} else if (j.is_object()) {
		// Single record
		for (nlohmann::json::const_iterator it = j.begin(); it != j.end(); ++it) {
			dpc.columns.push_back(it.key());
		}
		DataPoint dp;
		from_json(j, dp);
		dpc.records.push_back(std::move(dp));
	}
}

DataPointCollection parse_csv_data(std::string_view csv_text) {
	return parse_delimited(csv_text, DelimitedParser::Delimiter::Comma);
}

DataPointCollection parse_ssv_data(std::string_view ssv_text) {
	return parse_delimited(ssv_text, DelimitedParser::Delimiter::Space);
}

} // namespace ncei
