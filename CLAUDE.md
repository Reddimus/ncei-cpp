# Development

Run make build, make test, make lint, and make consumers. make debug uses
build-debug; sanitizers require -DNCEI_ENABLE_SANITIZERS=ON. make bench is
offline. python3 tools/test_install_hooks.py verifies custom-hook preservation.

C++23, namespace ncei, tabs, 100 columns. Use explicit local types except
iterators, structured bindings, and lambda closures.

Layers: core, HTTP, models, CDO/Data Service clients, ncei::ncei umbrella.
Glaze 9 is private. Use src/json.hpp for complete bounded JSON; null scalar
fields retain defaults. Successful reads replace previous model outputs.

CDO uses a token header, one-based pagination, five requests/second and
10,000/day per token. Every retry consumes both budgets. UTC daily counters
cover this limiter only. Data Service needs no token. Encode opaque URL
components with src/url.hpp; keep CSV separators outside encoded items.

Clients need external synchronization; caches and token buckets are thread-safe.
Date helpers validate input and may throw; get_data_all converts invalid dates
to Result errors. Raw PDF/NetCDF download does not imply decoding support.

See README for dependency/compiler floors. CI builds/tests Linux, macOS,
and Windows. Merge passing PRs before signed version tags trigger releases.
