// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
#pragma once

/// @file monitoring_client.hpp
/// @brief Authoritative national-climate settlement feeds (no auth).
///
/// Thin client over the multi-host static feeds that polymarket
/// national-climate contracts settle on:
///  - NCEI Climate at a Glance (NOAAGlobalTemp) — "hottest year rank"
///  - NASA GISTEMP — independent global-anomaly cross-check
///  - NSIDC Sea Ice Index v4 — "minimum Arctic sea-ice extent"
///
/// All keyless; relies on the HttpClient absolute-URL passthrough.

#include "ncei/error.hpp"
#include "ncei/http_client.hpp"
#include "ncei/models/climate_index.hpp"
#include "ncei/retry.hpp"

#include <memory>
#include <string>

namespace ncei {

class MonitoringClient {
public:
	struct Config {
		ClientConfig http;
		RetryPolicy retry{};
	};

	explicit MonitoringClient(Config config);
	~MonitoringClient();
	MonitoringClient(MonitoringClient&&) noexcept;
	MonitoringClient& operator=(MonitoringClient&&) noexcept;
	MonitoringClient(const MonitoringClient&) = delete;
	MonitoringClient& operator=(const MonitoringClient&) = delete;

	/// NCEI Climate at a Glance global land+ocean annual anomaly series
	/// (`.../globe/land_ocean/12/1/{start}-{end}/data.json`).
	[[nodiscard]] Result<CagSeries> get_cag_global_annual(int start_year, int end_year);

	/// NASA GISTEMP global Land-Ocean Temperature Index (annual J-D).
	[[nodiscard]] Result<GistempSeries> get_gistemp_global();

	/// NSIDC Sea Ice Index v4 Arctic extent. `monthly` false = daily file;
	/// `month` selects the monthly file (default 9 = the September minimum
	/// that annual "minimum Arctic sea-ice" contracts settle on).
	[[nodiscard]] Result<SeaIceSeries> get_nsidc_arctic_extent(bool monthly = true, int month = 9);

	[[nodiscard]] HttpClient& http_client();
	[[nodiscard]] const HttpClient& http_client() const;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;

	[[nodiscard]] Result<HttpResponse> fetch(const std::string& url);
};

} // namespace ncei
