// Copyright (c) 2026 PredictionMarketsAI
// SPDX-License-Identifier: MIT
//
// Microbenchmark: parse a representative NCEI CDO `/stations` list response
// 1k times and report wall-clock. Used as a parse-throughput regression
// guard with `ctest --timeout` and an absolute upper bound on us/op.
//
// Historical baseline (recorded at migration time on x86_64-v3, GCC 13.3,
// -O3 -DNDEBUG, payload=~21KB, iters=1000):
//
//     nlohmann/json v3.11.3 :  ~360-590 us/op  (pre-migration baseline)
//     glaze v7.6.0          :   ~32-40  us/op  (post-migration)
//     speedup               :   9-15x
//
// The CDO list-response shape is allocation-heavy on the nlohmann path
// (every key gets its own std::string + ordered-map slot), which is where
// most of the gap comes from. The pre-migration nlohmann bench lived in
// tests/parse_benchmark.cpp at the time of the Glaze migration commit; it
// has since been removed along with the nlohmann FetchContent dep.
// Re-introduce a side-by-side bench only if a future regression suspicion
// warrants it.

#include "ncei/models/cdo/station.hpp"
#include "ncei/models/common.hpp"
#include "ncei/pagination.hpp"

#include <chrono>
#include <cstdio>
#include <string>

namespace {

// 100 CDOStation records wrapped in the standard CDO `{metadata, results}`
// envelope. ~21 KB of JSON — close to what a real `/stations?limit=100`
// call returns from the CDO API.
std::string make_payload() {
	std::string json;
	json.reserve(32 * 1024);
	json += R"({"metadata":{"resultset":{"offset":0,"count":100,"limit":100}},"results":[)";
	constexpr int kRecords = 100;
	for (int i = 0; i < kRecords; ++i) {
		if (i != 0) {
			json += ',';
		}
		char buf[512];
		std::snprintf(buf, sizeof(buf),
					  R"({"id":"GHCND:USW%011d","name":"STATION NAME %d","datacoverage":%f,)"
					  R"("mindate":"1948-01-01","maxdate":"2024-12-31","latitude":%f,)"
					  R"("longitude":%f,"elevation":%f,"elevationUnit":"METERS"})",
					  i, i, 0.9 + (i % 10) * 0.01, 25.0 + i * 0.1, -120.0 + i * 0.5,
					  10.0 + i * 1.5);
		json += buf;
	}
	json += "]}";
	return json;
}

} // namespace

int main() {
	const std::string payload = make_payload();
	constexpr int kIterations = 1000;

	// Warmup — let the allocator and CPU settle.
	for (int i = 0; i < 50; ++i) {
		ncei::CDOResponse<ncei::CDOStation> warm;
		(void)ncei::deserialize_cdo_list(payload, warm);
	}

	std::chrono::nanoseconds glaze_total{0};
	std::size_t glaze_checksum = 0;
	for (int i = 0; i < kIterations; ++i) {
		ncei::CDOResponse<ncei::CDOStation> resp;
		std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
		ncei::Result<void> r = ncei::deserialize_cdo_list(payload, resp);
		std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
		if (!r) {
			std::fprintf(stderr, "glaze parse failed: %s\n", r.error().message.c_str());
			return 1;
		}
		glaze_total += (t1 - t0);
		glaze_checksum += resp.results.size();
	}

	if (glaze_checksum != static_cast<std::size_t>(100) * kIterations) {
		std::fprintf(stderr, "checksum mismatch: glaze=%zu (expected %d)\n", glaze_checksum,
					 100 * kIterations);
		return 1;
	}

	const double glaze_ms = glaze_total.count() / 1e6;
	const double us_per_op = (glaze_total.count() / 1e3) / kIterations;

	std::printf("parse_benchmark: payload=%zuB iters=%d\n", payload.size(), kIterations);
	std::printf("  glaze: %8.3f ms total  (%8.3f us/op)\n", glaze_ms, us_per_op);

	// Regression guard: at migration time, Glaze parsed this payload at
	// ~32-40 us/op on x86_64-v3. Set the cap at 200 us/op — that's still
	// faster than the nlohmann baseline (~360-590 us/op) and leaves a
	// healthy slack window for slower CI runners and Debug builds.
	constexpr double kMaxUsPerOp = 200.0;
	if (us_per_op > kMaxUsPerOp) {
		std::fprintf(stderr, "REGRESSION: %.3f us/op exceeds cap of %.0f us/op\n", us_per_op,
					 kMaxUsPerOp);
		return 1;
	}
	return 0;
}
