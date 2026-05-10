# Changelog

All notable changes to **ncei-cpp** are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project
uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
  ` `code` ` spans survive
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

[Unreleased]: https://github.com/Reddimus/ncei-cpp/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Reddimus/ncei-cpp/releases/tag/v0.1.0
