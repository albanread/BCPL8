#!/bin/bash

set -e

RUNTIME_DIR="runtime"
BUILD_DIR="${RUNTIME_DIR}/build"
TOP_LEVEL_DIR="."
ERRORS_FILE="errors.txt"

# Options
BUILD_STANDALONE=ON
BUILD_TEST=OFF # Set to ON if you want to build the runtime test executable

echo "Building BCPL runtime libraries using CMake..."

# Clear errors file
> "${ERRORS_FILE}"

# Create build directory if it doesn't exist
mkdir -p "${BUILD_DIR}"

# Run CMake configuration
cmake -S "${RUNTIME_DIR}" -B "${BUILD_DIR}" \
    -DBCPL_BUILD_STANDALONE_RUNTIME=${BUILD_STANDALONE} \
    -DBCPL_BUILD_RUNTIME_TEST=${BUILD_TEST} 2>> "${ERRORS_FILE}"

# Build all targets
cmake --build "${BUILD_DIR}" 2>> "${ERRORS_FILE}"

# Copy built libraries to top-level NewBCPL folder
cp "${BUILD_DIR}/libbcpl_runtime_jit.a" "${TOP_LEVEL_DIR}/" 2>> "${ERRORS_FILE}"
if [ "${BUILD_STANDALONE}" = "ON" ]; then
    cp "${BUILD_DIR}/libbcpl_runtime_c.a" "${TOP_LEVEL_DIR}/" 2>> "${ERRORS_FILE}"
fi
if [ "${BUILD_TEST}" = "ON" ]; then
    cp "${BUILD_DIR}/runtime_test" "${TOP_LEVEL_DIR}/" 2>> "${ERRORS_FILE}"
fi

echo "Runtime build complete. Libraries copied to ${TOP_LEVEL_DIR}/"
