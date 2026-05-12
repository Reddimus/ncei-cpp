# ncei-cpp Development Guide

## Build & Test

```bash
make build          # Release build (CMake + make)
make debug          # Debug build
make test           # Run unit tests (ctest)
make lint           # Check formatting (clang-format --dry-run)
make format         # Format in place
make coverage       # lcov coverage report
make clean          # Remove build/
```

## Architecture

- **Layered static libraries**: ncei_core -> ncei_http -> ncei_models -> ncei_cdo/ncei_data -> ncei (INTERFACE)
- **C++23**: `std::expected<T, Error>` for all returns, no exceptions
- **Two clients**: CDOClient (token auth, rate limited, paginated) + DataServiceClient (no auth, multi-format)
- **Patterns**: Pimpl (HttpClient, CDOClient, DataServiceClient), non-copyable/movable, `[[nodiscard]]`
- **JSON**: [Glaze](https://github.com/stephenberry/glaze) v7.6.0 via FetchContent (compile-time reflection, ~9-15x parse speedup over nlohmann on the CDO list-response shape — migrated 2026-05-11). Public entry points are the `deserialize_*(std::string_view, T&) -> Result<void>` family in `include/ncei/models/common.hpp`; per-T `glz::meta` specializations live in each model `.cpp`. The `skip_null_members_on_read = true` opt is wired through `ncei::detail::kReadOpts` so CDO's frequent `"datacoverage": null` rows leave the field at its default. Dynamic-key payloads (DataPoint's user-driven TMAX/TMIN/PRCP columns) use `glz::generic` — search for `TODO(glaze):` markers. See `tests/parse_benchmark.cpp` for the regression guard.
- **Tests**: GoogleTest via FetchContent. Fixture files in `tests/fixtures/`.

## Conventions

- Code style: `.clang-format` (LLVM base, tabs, 100 cols)
- Namespace: `ncei`
- **No `auto`**: Use explicit types. `auto` is only acceptable for iterators, structured bindings (`auto& [key, val]`), and range-for loops (`const auto& x : container`).
- Model structs are declared in `include/ncei/models/...`, with `glz::meta` specializations and the `deserialize_*` implementations in matching `src/models/.../*.cpp` files. The pre-migration `from_json(const nlohmann::json&, T&)` overloads have been removed; downstream consumers use the high-level client methods (`CDOClient::get_*`, `DataServiceClient::get_*`), never these helpers directly.
- Include order: project headers first, then system headers (enforced by clang-format).

## CDO API Notes

- Token via `CDOClient::Config::token`, sent as `token` HTTP header
- Rate limit: 5 req/sec (token bucket) + 10,000/day (daily counter)
- Pagination: offset-based, max 1000 per page
- Date range limit: 1 year for daily/hourly, 10 years for monthly/annual
- `get_data_all()` handles both auto-pagination and date-splitting

## Data Service API Notes

- No authentication required
- Default format: CSV (most reliable), also JSON, SSV
- `+` in timezone offsets must be URL-encoded as `%2B`
