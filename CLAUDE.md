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
- **JSON**: nlohmann/json via FetchContent. Use `json_string()` / `json_int()` / `json_double()` / `json_bool()` helpers from `models/common.hpp`.
- **Tests**: GoogleTest via FetchContent. Fixture files in `tests/fixtures/`.

## Conventions

- Code style: `.clang-format` (LLVM base, tabs, 100 cols)
- Namespace: `ncei`
- **No `auto`**: Use explicit types. `auto` is only acceptable for iterators, structured bindings (`auto& [key, val]`), and range-for loops (`const auto& x : container`).
- All model `from_json` functions use the null-safe helpers, NOT `j.value("key", "")`.
- Models declare `from_json` in headers, implement in `.cpp` files.
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
