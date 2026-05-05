#!/usr/bin/env sh
# Copyright (c) 2026, The Arqma Network
# Configure and build unit_tests with BUILD_TESTS=ON (Git Bash / MSYS / Unix).
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:-$ROOT/build-tests-pulse}"
echo "Source: $ROOT"
echo "Build:  $OUT"
# Match CI: legacy unit_tests .cpp files do not compile against current core; slim Pulse-only binary.
cmake -S "$ROOT" -B "$OUT" -D BUILD_TESTS=ON -D ARQMA_CI_PULSE_GTEST=ON
cmake --build "$OUT" --target unit_tests --parallel
echo "Done. Run: \"$OUT/tests/unit_tests\" --gtest_filter=Pulse*"
echo "        (path may be \"$OUT/bin/unit_tests\" on some generators)"
