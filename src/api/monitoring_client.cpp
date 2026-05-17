// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT

#include "ncei/monitoring_client.hpp"

#include <format>
#include <string>
#include <utility>

namespace ncei {

struct MonitoringClient::Impl {
	HttpClient http;
	RetryPolicy retry;

	Impl(ClientConfig http_config, RetryPolicy rp)
		: http(std::move(http_config)), retry(std::move(rp)) {}
};

MonitoringClient::MonitoringClient(Config config) {
	// base_url stays empty — every feed is requested as an absolute URL
	// (HttpClient passes those through unchanged).
	impl_ = std::make_unique<Impl>(std::move(config.http), config.retry);
}

MonitoringClient::~MonitoringClient() = default;
MonitoringClient::MonitoringClient(MonitoringClient&&) noexcept = default;
MonitoringClient& MonitoringClient::operator=(MonitoringClient&&) noexcept = default;

HttpClient& MonitoringClient::http_client() {
	return impl_->http;
}

const HttpClient& MonitoringClient::http_client() const {
	return impl_->http;
}

Result<HttpResponse> MonitoringClient::fetch(const std::string& url) {
	Result<HttpResponse> r =
		with_retry([&]() -> Result<HttpResponse> { return impl_->http.get(url); }, impl_->retry);
	if (!r) {
		return std::unexpected(r.error());
	}
	if (r->status_code != 200) {
		return std::unexpected(Error::from_response(r->status_code, r->body));
	}
	return r;
}

Result<CagSeries> MonitoringClient::get_cag_global_annual(int start_year, int end_year) {
	const std::string url =
		std::format("https://www.ncei.noaa.gov/access/monitoring/climate-at-a-glance/global/"
					"time-series/globe/land_ocean/12/1/{}-{}/data.json",
					start_year, end_year);
	Result<HttpResponse> resp = fetch(url);
	if (!resp) {
		return std::unexpected(resp.error());
	}
	CagSeries out;
	Result<void> parse = deserialize_cag_series(resp->body, out);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return out;
}

Result<GistempSeries> MonitoringClient::get_gistemp_global() {
	Result<HttpResponse> resp =
		fetch("https://data.giss.nasa.gov/gistemp/tabledata_v4/GLB.Ts+dSST.csv");
	if (!resp) {
		return std::unexpected(resp.error());
	}
	GistempSeries out;
	Result<void> parse = deserialize_gistemp_csv(resp->body, out);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return out;
}

Result<SeaIceSeries> MonitoringClient::get_nsidc_arctic_extent(bool monthly, int month) {
	std::string url;
	if (monthly) {
		url = std::format("https://noaadata.apps.nsidc.org/NOAA/G02135/north/monthly/"
						  "data/N_{:02d}_extent_v4.0.csv",
						  month);
	} else {
		url = "https://noaadata.apps.nsidc.org/NOAA/G02135/north/daily/data/"
			  "N_seaice_extent_daily_v4.0.csv";
	}
	Result<HttpResponse> resp = fetch(url);
	if (!resp) {
		return std::unexpected(resp.error());
	}
	SeaIceSeries out;
	out.region = "arctic";
	Result<void> parse = deserialize_nsidc_sea_ice_csv(resp->body, monthly, out);
	if (!parse) {
		return std::unexpected(parse.error());
	}
	return out;
}

} // namespace ncei
