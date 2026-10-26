# Changelog

All notable changes to **ncei-cpp** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

- Enforce the Release parse benchmark limit when CMake supplies an empty argument.

## [0.4.0] - 2026-10-25

### Fixed

- Include the final record in one-based CDO pagination; bound offset arithmetic.
- Stop non-progressing pagination and charge every retry against rate/daily quotas.
- Validate complete bounded JSON and replace reused model outputs.
- Correct quoted CSV endings, climate numeric parsing, and metadata dataTypes/scaling.
- Encode opaque query values and path IDs; validate dates and chunk ranges.
- Roll back failed cache insertions and disable zero-capacity/TTL caches.
- Bound HTTP bodies and protocols; contain callback allocation failures.
- Normalize token settings and retry delays without arithmetic overflow.
- Preserve parent build flags and custom hooks; run benchmarks offline.

### Changed

- Require CMake 3.31 and thread-safe libcurl 7.85+; update Glaze to 9.0.0.
- Default tests, examples, and LTO to OFF for embedded builds.
- Add installed/FetchContent consumers and portable loopback HTTP regressions.

## [0.3.1] - 2026-05-19

### Fixed

- **NCEI CAG response format flip.** Upstream NCEI Climate at a Glance
  `data.json` (`/access/monitoring/climate-at-a-glance/global/...`) now
  wraps each year's value in an object envelope:

      old:  "1851": "-0.12"          (bare string)
      new:  "1851": {"departure": -0.12}   (nested-object form)

  `deserialize_cag_series` previously matched only the bare string /
  numeric forms, so every row dropped silently and the parser raised
  `CAG: empty data series`. As a result the polymarket-data
  `climate-index` ingester (#79) couldn't populate the `ncei_cag`
  series — gistemp + nsidc continued to work; the CAG cross-check did
  not. Detected 2026-05-19 via the climate-index startup log.

  The deserializer now tolerates BOTH forms: the legacy bare
  string/number stays accepted (so a future NCEI flip back doesn't
  re-break us), and the new `{"departure": <num|str>}` envelope is
  unwrapped. Any other inner keys (e.g. a future `"rank"` /
  `"uncertainty"`) are ignored. Tests in
  `tests/test_climate_index.cpp::ParsesCagDepartureObjectForm` /
  `ParsesCagDepartureStringInsideObject` /
  `RejectsCagObjectWithoutDeparture` pin all three regimes.

## [0.3.0] - 2026-05-17

### Added

- **National-climate settlement feeds.** `ncei/models/climate_index.hpp`
  with `deserialize_cag_series` (NCEI Climate at a Glance / NOAAGlobalTemp
  `data.json` — "hottest year rank"), `deserialize_gistemp_csv` (NASA
  GISTEMP `GLB.Ts+dSST.csv` cross-check), and
  `deserialize_nsidc_sea_ice_csv` (NSIDC Sea Ice Index v4 — "minimum
  Arctic sea-ice extent"). New `MonitoringClient`
  (`get_cag_global_annual` / `get_gistemp_global` /
  `get_nsidc_arctic_extent`). `HttpClient` now passes absolute URLs
  through unchanged so one client reaches all three hosts (ncei.noaa.gov,
  data.giss.nasa.gov, noaadata.apps.nsidc.org); relative CDO / Data
  Service paths are unaffected. Consumed by polymarket-data (Phase 3c).
- `.editorconfig` (fleet-standard: tabs, 4-width, LF, UTF-8, 100-col
  max for C++). Sibling to `.clang-format`; covers editors that don't
  read `.clang-format` (#12).
- `make pre-commit` + `make install-hooks` targets — auto-format + lint
  via git pre-commit hook (#8).

### Changed

- README install snippet pinned to v0.2.0 (#9, #10).

### Fixed

- LICENSE: copyright holder de-truncated (#11).

## [0.2.0] - 2026-05-12

### Changed

- **BREAKING**: JSON parsing migrated from `nlohmann/json` v3.11.3 to
  [`Glaze`](https://github.com/stephenberry/glaze) v7.6.0. The public
  client API (`CDOClient::get_*`, `DataServiceClient::get_*`,
  `parse_csv_data` / `parse_ssv_data`) is unchanged. Internal
  `from_json(const nlohmann::json&, T&)` overloads have been replaced
  with `deserialize_<T>(std::string_view, T&) -> Result<void>` in the
  `ncei::` namespace. The transitional `json_string` / `json_int` /
  `json_double` / `json_bool` helpers in `models/common.hpp` are gone
  (no external consumers). Benchmark: ~9-15x parse speedup on a
  representative 21 KB CDO `/stations` list-response payload
  (nlohmann ~360-590 us/op → Glaze ~32-40 us/op on x86_64-v3,
  GCC 13.3, -O3 -DNDEBUG).
- C++23 baseline reaffirmed — Glaze requires C++23 for its
  compile-time reflection path. `CMakeLists.txt` already enforced this.

### Added

- `tests/glaze_test.cpp` — verifies parse-output shape parity with the
  pre-migration behavior (null-safety on every scalar field,
  unknown-key tolerance, dynamic DataPoint attribute preservation,
  CDO list-response envelope walking, snake_case ↔ camelCase
  JSON-key aliasing for `datacoverage` / `mindate` / `maxdate` /
  `elevationUnit`).
- `tests/parse_benchmark.cpp` — parse-throughput regression guard. Caps
  at 200 us/op (≈3x slower than the migration-time Glaze number) with
  a 30s ctest timeout.

### Removed

- `src/core/pagination.cpp` (the nlohmann `from_json` overload for
  `ResultSetMetadata`). The CDO envelope is now parsed by a templated
  Glaze meta specialization in `src/models/pagination_detail.hpp` (an
  internal-only header).

## [0.1.1] - 2026-05-10

### CI

- First-ever GitHub Actions workflow coverage — build + test + lint on
  Ubuntu 24.04, build-only on macos-latest, markdown-lint via
  `DavidAnson/markdownlint-cli2-action`
  ([`3031c1e`](https://github.com/Reddimus/ncei-cpp/commit/3031c1e)).
- `build-windows` job added via vcpkg (parity with the rest of the SDK
  family).
- `release.yml` auto-creates a GitHub Release on `vX.Y.Z` tag push,
  with body sourced from this CHANGELOG via `--notes-file` so inline
  inline code spans survive
  ([`6831030`](https://github.com/Reddimus/ncei-cpp/commit/6831030),
  [`91f53ca`](https://github.com/Reddimus/ncei-cpp/commit/91f53ca)).
- Tag/CMakeLists `VERSION` drift is caught at release time
  ([`67fa6de`](https://github.com/Reddimus/ncei-cpp/commit/67fa6de)).
- `actions/checkout@v6` upgrade for Node 24 runtime
  ([`71f41f1`](https://github.com/Reddimus/ncei-cpp/commit/71f41f1)).
- `.markdownlint-cli2.yaml` config disables `MD013` (line-length) and
  other style-noise rules; sibling repos share the same shape.
- `MD004` (asterisk style) disabled for prose continuation parity
  ([`79abd8c`](https://github.com/Reddimus/ncei-cpp/commit/79abd8c)).
- `CLAUDE.md` markdown reflowed to satisfy the new lint job
  ([`c4add51`](https://github.com/Reddimus/ncei-cpp/commit/c4add51)).
- `cpp_auto_audit.py` now walks `--cached + --others` so new test
  files don't pass local lint and fail CI
  ([`d681291`](https://github.com/Reddimus/ncei-cpp/commit/d681291)).

### Docs

- Add a Contributing section to the README
  ([`c2c15a1`](https://github.com/Reddimus/ncei-cpp/commit/c2c15a1)).
- Add CI / release / C++ standard / license badges to the top of the
  README ([`aad8888`](https://github.com/Reddimus/ncei-cpp/commit/aad8888)).
- Squash double-blank-line inserted by the Contributing section
  ([`898c594`](https://github.com/Reddimus/ncei-cpp/commit/898c594)).
- Add this `CHANGELOG.md` + the auto-release-on-tag workflow it feeds
  ([`6831030`](https://github.com/Reddimus/ncei-cpp/commit/6831030)).

### Build

- Enforce explicit local cpp types ([`72ab089`](https://github.com/Reddimus/ncei-cpp/commit/72ab089)).

### Chore

- Production-hardening `.gitignore` patterns mirrored from the rest of
  the SDK family
  ([`bca2e70`](https://github.com/Reddimus/ncei-cpp/commit/bca2e70)).

### Refactor

- Replace `std::ostringstream` with `std::format` / `std::format_to`
  throughout ([`91de476`](https://github.com/Reddimus/ncei-cpp/commit/91de476)).

## [0.1.0] — 2026-04-15

### Added

- Initial release: C++23 NCEI Climate Data SDK
- Dual-client design:
  - `CDOClient` — token auth, 5 req/sec + 10K/day rate limit, offset
    pagination, 1-year date-range limit (auto-split via `get_data_all`)
  - `DataServiceClient` — no auth, multi-format (CSV / JSON / SSV),
    `+` URL-encoding for timezones
- Layered static libraries: `ncei_core` → `ncei_http` → `ncei_models`
  → `ncei_cdo` + `ncei_data` → `ncei` (INTERFACE)
- `std::expected<T, Error>` for all returns; `ErrorCode::QuotaExceeded`
  for CDO daily-quota exhaustion
- Auto-pagination + auto-date-splitting (transparent multi-year queries)
- Optional NetCDF support via `NCEI_ENABLE_NETCDF` build flag
- Endpoints: CDO (datasets, data_categories, data_types,
  location_categories, locations, stations, data) + Data Service
  (data, metadata)

[Unreleased]: https://github.com/Reddimus/ncei-cpp/compare/v0.4.0...HEAD
[0.4.0]: https://github.com/Reddimus/ncei-cpp/compare/v0.3.1...v0.4.0
[0.3.1]: https://github.com/Reddimus/ncei-cpp/compare/v0.3.0...v0.3.1
[0.3.0]: https://github.com/Reddimus/ncei-cpp/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/Reddimus/ncei-cpp/compare/v0.1.1...v0.2.0
[0.1.1]: https://github.com/Reddimus/ncei-cpp/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/Reddimus/ncei-cpp/releases/tag/v0.1.0
