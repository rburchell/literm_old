#!/bin/sh

# where coverage data resides.
BUILD_DIR="${1}"

# where we want output.
RESULTS_DIR="${1}/html"

mkdir -p ${RESULTS_DIR}

lcov -d "${BUILD_DIR}" -c -o "${BUILD_DIR}/coverage.info"

# remove some paths to make the results sane
lcov -r "${BUILD_DIR}/coverage.info" "*/qt/*" "*Xcode.app*" "*.moc" "*moc_*.cpp" "*/tests/*" -o "${BUILD_DIR}/coverage-filtered.info"

genhtml -o "${RESULTS_DIR}" "${BUILD_DIR}/coverage-filtered.info"

lcov -d "${BUILD_DIR}" -z

open "${RESULTS_DIR}/index.html"
