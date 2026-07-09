#!/usr/bin/env sh
set -eu

repo_root="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
build_dir="${1:-}"
jobs="${BUILD_PARALLEL_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

if [ -z "$build_dir" ]; then
  for candidate in \
    "$repo_root/build/Desktop_Qt_6_11_1_MinGW_64_bit-Debug/game" \
    "$repo_root/build/game"
  do
    if [ -f "$candidate/Makefile" ]; then
      build_dir="$candidate"
      break
    fi
  done
fi

if [ -z "$build_dir" ] || [ ! -f "$build_dir/Makefile" ]; then
  echo "Build directory with Makefile not found." >&2
  exit 1
fi

echo "Building in: $build_dir"
echo "Parallel jobs: $jobs"
make -C "$build_dir" -j"$jobs"
