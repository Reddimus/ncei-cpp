// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file climate_index.hpp
/// @brief Authoritative settlement series for national-climate contracts.
///
/// Polymarket national-climate markets settle on:
///  - "hottest year rank" / global-temperature-anomaly  -> NCEI Climate
///    at a Glance (NOAAGlobalTemp) JSON + NASA GISTEMP CSV (cross-check).
///  - "minimum Arctic sea-ice extent"  -> NSIDC Sea Ice Index v4 CSV.
///
/// These are thin JSON/CSV shapes — pure `deserialize_*` parsers here;
/// the fetch convenience lives in `ncei/monitoring_client.hpp`.

#include "ncei/error.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace ncei {

/// One period -> value sample (period is "YYYY" or "YYYY-MM" or ISO date).
struct ClimatePoint {
	std::string period;
	double value{0.0};
};

/// NCEI Climate at a Glance global time series (`.../data.json`).
struct CagSeries {
	std::string title;
	std::string units;
	std::string base_period;
	std::vector<ClimatePoint> data; ///< annual (or monthly) anomalies
};

/// NASA GISTEMP global Land-Ocean Temperature Index annual means
/// (`GLB.Ts+dSST.csv`, the `J-D` column).
struct GistempSeries {
	std::vector<ClimatePoint> annual; ///< year -> anomaly (base 1951-1980)
};

/// NSIDC Sea Ice Index v4 extent series (G02135 monthly or daily CSV).
struct SeaIceSeries {
	bool monthly{true};
	std::string region{"arctic"};
	std::vector<ClimatePoint> extent; ///< period -> extent (million km^2)
};

/// Parse an NCEI Climate-at-a-Glance `data.json`
/// (`{"description":{title,units,base_period,...},"data":{"1880":"-0.19",...}}`).
[[nodiscard]] Result<void> deserialize_cag_series(std::string_view body, CagSeries& out);

/// Parse a NASA GISS `GLB.Ts+dSST.csv` (skips the banner rows, reads the
/// `Year` and `J-D` columns; "***" / blank annual cells are dropped).
[[nodiscard]] Result<void> deserialize_gistemp_csv(std::string_view body, GistempSeries& out);

/// Parse an NSIDC Sea Ice Index v4 CSV. Monthly file columns:
/// `year, mo, data-type, region, extent, area`; daily file columns:
/// `Year, Month, Day, Extent, Missing, Source Data`.
[[nodiscard]] Result<void> deserialize_nsidc_sea_ice_csv(std::string_view body, bool monthly,
														 SeaIceSeries& out);

} // namespace ncei
