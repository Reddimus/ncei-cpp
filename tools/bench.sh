#!/usr/bin/env bash
# Parsing throughput only; this benchmark makes no network requests.
set -euo pipefail
binary="${BUILD_DIR:-build}/tests/ncei_parse_benchmark"
if [ ! -x "$binary" ]; then
    echo "Build tests first: make build"
    exit 1
fi
exec "$binary" "$@"
