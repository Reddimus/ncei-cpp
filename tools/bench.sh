#!/usr/bin/env bash
set -euo pipefail

ITERATIONS="${1:-254}"
BINARY="./build/examples/example_basic_cdo"

if [ ! -f "$BINARY" ]; then
    echo "Build first: make build"
    exit 1
fi

echo "Benchmarking $BINARY ($ITERATIONS iterations)..."

total=0
for i in $(seq 1 "$ITERATIONS"); do
    start=$(date +%s%N)
    $BINARY > /dev/null 2>&1
    end=$(date +%s%N)
    elapsed=$(( (end - start) / 1000000 ))
    total=$((total + elapsed))
done

avg=$((total / ITERATIONS))
echo "Total: ${total}ms | Avg: ${avg}ms | Iterations: $ITERATIONS"
