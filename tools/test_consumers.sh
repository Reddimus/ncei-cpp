#!/usr/bin/env bash
# Verify the installed package and FetchContent defaults without weather API requests.
set -euo pipefail
source_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
scratch_dir=$(mktemp -d "${TMPDIR:-/tmp}/ncei-consumers.XXXXXX")
trap 'rm -rf "$scratch_dir"' EXIT
version=$(sed -nE 's/^project\(ncei-cpp VERSION ([0-9.]+) LANGUAGES CXX\)$/\1/p' "$source_dir/CMakeLists.txt")
test -n "$version"
mkdir "$scratch_dir/readme"
python3 - "$source_dir/README.md" "$scratch_dir/readme" <<'PYCODE'
import sys
from pathlib import Path
for index, block in enumerate(Path(sys.argv[1]).read_text().split("```cpp\n")[1:]):
    Path(sys.argv[2], f"example{index}.cpp").write_text(block.split("```", 1)[0])
PYCODE

cmake -S "$source_dir" -B "$scratch_dir/sdk" -DCMAKE_BUILD_TYPE=Release \
    -DNCEI_BUILD_TESTS=ON -DNCEI_BUILD_EXAMPLES=OFF -DNCEI_ENABLE_LTO=OFF \
    -DCMAKE_INSTALL_PREFIX="$scratch_dir/prefix" "$@"
cmake --build "$scratch_dir/sdk" --target ncei_cdo ncei_data --parallel 4
cmake --install "$scratch_dir/sdk"
for dependency in glaze gtest gmock; do
    if [ -e "$scratch_dir/prefix/include/$dependency" ]; then
        echo "SDK install leaked $dependency headers" >&2
        exit 1
    fi
done
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/installed" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$scratch_dir/prefix" \
    -DNCEI_EXPECTED_VERSION="$version" -DREADME_EXAMPLES_DIR="$scratch_dir/readme"
cmake --build "$scratch_dir/installed" --parallel 4
"$scratch_dir/installed/consumer"
cmake -S "$source_dir/tests/consumer" -B "$scratch_dir/fetchcontent" \
    -DCMAKE_BUILD_TYPE=Release -DNCEI_SOURCE_DIR="$source_dir" -DREADME_EXAMPLES_DIR="$scratch_dir/readme" "$@"
cmake --build "$scratch_dir/fetchcontent" --parallel 4
"$scratch_dir/fetchcontent/consumer"
echo "Installed and FetchContent consumers pass for ncei-cpp $version"
