# Changelog

All notable changes to **ncei-cpp** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### CI

- Add `.markdownlint-cli2.yaml` mirroring the open-meteo-cpp config —
  disables `MD013` (line-length) and other style-noise rules but keeps
  `MD022`/`MD031`/`MD032` formatting hygiene enforced.
- Fix `CLAUDE.md` formatting (blank lines around headings, fenced
  blocks, and section breaks) so the markdown-lint job passes.

### Build

- Enforce explicit local cpp types ([`72ab089`](https://github.com/Reddimus/ncei-cpp/commit/72ab089)).

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

[Unreleased]: https://github.com/Reddimus/ncei-cpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Reddimus/ncei-cpp/releases/tag/v0.1.0
