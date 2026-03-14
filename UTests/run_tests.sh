#!/usr/bin/env bash
# ============================================================
# run_tests.sh
# Build and run all unit tests for MultiMediaApp.
#
# Usage (from project root):
#   chmod +x tests/run_tests.sh
#   ./UTests/run_tests.sh              # run all tests
#   ./UTests/run_tests.sh TestSQLite   # run one test suite
#   ./UTests/run_tests.sh --verbose    # full QtTest output
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_tests"
FILTER="${1:-}"
VERBOSE=0

if [[ "${FILTER}" == "--verbose" ]]; then
    VERBOSE=1
    FILTER=""
fi

echo "=== Building tests ==="
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake "${SCRIPT_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DQT_QPA_PLATFORM=offscreen \
    -G Ninja 2>/dev/null || cmake "${SCRIPT_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake --build . -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"

echo ""
echo "=== Running tests ==="

if [[ -n "${FILTER}" ]]; then
    # Run a single named test binary with full QtTest output
    if [[ -f "./${FILTER}" ]]; then
        echo "--- ${FILTER} ---"
        QT_QPA_PLATFORM=offscreen "./${FILTER}" -v2
    else
        echo "Test binary '${FILTER}' not found in ${BUILD_DIR}"
        exit 1
    fi
else
    # Run all tests via CTest
    CTEST_ARGS="--output-on-failure"
    if [[ "${VERBOSE}" -eq 1 ]]; then
        CTEST_ARGS="${CTEST_ARGS} -V"
    fi

    QT_QPA_PLATFORM=offscreen ctest ${CTEST_ARGS}
fi

echo ""
echo "=== Done ==="
