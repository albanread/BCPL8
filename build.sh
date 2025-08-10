#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define build directories and error log file
BUILD_DIR="build"
OBJ_DIR="${BUILD_DIR}/obj"
BIN_DIR="${BUILD_DIR}/bin"
ERRORS_FILE="errors.txt"
MAX_JOBS=8 # Number of parallel compilation jobs

# Clear the errors file at the start of the build
> "${ERRORS_FILE}"

# --- Argument Parsing ---
CLEAN_BUILD=false
for arg in "$@"; do
    if [ "$arg" == "--clean" ]; then
        CLEAN_BUILD=true
        break
    fi
done

# --- Build Setup ---
if ${CLEAN_BUILD}; then
    echo "Performing clean build..."
    echo "Cleaning previous build artifacts (excluding runtime libraries)..."
    # Preserve any existing runtime libraries before cleaning
    mkdir -p temp_lib_backup
    if [ -f "libbcpl_runtime_jit.a" ]; then
        cp libbcpl_runtime_jit.a temp_lib_backup/
    fi
    if [ -f "libbcpl_runtime_c.a" ]; then
        cp libbcpl_runtime_c.a temp_lib_backup/
    fi

    rm -rf "${BUILD_DIR}"

    # Restore runtime libraries if they were backed up
    if [ -f "temp_lib_backup/libbcpl_runtime_jit.a" ]; then
        cp temp_lib_backup/libbcpl_runtime_jit.a ./
    fi
    if [ -f "temp_lib_backup/libbcpl_runtime_c.a" ]; then
        cp temp_lib_backup/libbcpl_runtime_c.a ./
    fi
    rm -rf temp_lib_backup
else
    echo "Performing incremental build..."
fi

mkdir -p "${OBJ_DIR}"
mkdir -p "${BIN_DIR}"
echo "Created build directories: ${OBJ_DIR} and ${BIN_DIR}"

# Increment version number (patch) before building
echo "Incrementing version number..."
python3 -c '
import re
import os
header = "version.cpp"
try:
    with open(header, "r") as f:
        lines = f.readlines()
    for i, line in enumerate(lines):
        m = re.match(r"#define BCPL_VERSION_PATCH (\\d+)", line)
        if m:
            patch = int(m.group(1)) + 1
            lines[i] = f"#define BCPL_VERSION_PATCH {patch}\\n"
            break
    with open(header, "w") as f:
        f.writelines(lines)
except FileNotFoundError:
    print(f"Error: {header} not found. Ensure the path is correct.")
    exit(1)
' 2>> "${ERRORS_FILE}" # Redirect python errors to errors.txt

# Find all potential source files
# Core files (excluding files that are explicitly added elsewhere)
CORE_SRC_FILES=$(find . -maxdepth 1 -name "*.cpp" ! -name "main.cpp" ! -name "live_*.cpp" ! -name "peephole_test.cpp" -print; \
                 find encoders -name "*.cpp" -print; \
                 find passes -name "*.cpp" -print; \
                 find linker_helpers -name "*.cpp" -print; \
                 find . -name "cf_*.cpp" -print; \
                 find . -name "rm_*.cpp" -print; \
                 find format -name "CodeFormatter.cpp" -print; \
                 echo "InstructionDecoder.cpp"; \
                 )

MAIN_SRC_FILE="main.cpp" # Assuming main.cpp exists and contains the main function

# Generated files from the 'generators' subfolder
GENERATED_SRC_FILES=$(find generators -name "gen_*.cpp" -print)

# Lexer-related files
LEXER_SRC_FILES=$(echo -e "lex_operator.cpp\nlex_scanner.cpp\nlex_tokens.cpp\nlex_utils.cpp")

# Analysis files from the 'analysis' subfolder
ANALYSIS_SRC_FILES=$(find analysis -name "*.cpp" -print)

# We don't need to add AZ_VISIT_SRC_FILES separately as they're already found by the analysis directory search

# Runtime is now built separately by buildruntime.sh
# We link to libbcpl_runtime_jit.a instead of compiling runtime files directly

# HeapManager files
HEAP_MANAGER_FILES=$(find HeapManager -name "*.cpp" -print 2>/dev/null || echo "")

# Live analysis files
LIVE_ANALYSIS_FILES=$(find . -name "live_*.cpp" -print 2>/dev/null || echo "")

# Signal handling files
SIGNAL_FILES="./SignalSafeUtils.cpp\n./SignalHandler.cpp"

# Combine all source files into a single list
ALL_SRC_FILES_RAW=$(echo -e "${CORE_SRC_FILES}\n${MAIN_SRC_FILE}\n${GENERATED_SRC_FILES}\n${ANALYSIS_SRC_FILES}\n${LEXER_SRC_FILES}\n${HEAP_MANAGER_FILES}\n${LIVE_ANALYSIS_FILES}\n${SIGNAL_FILES}")

# Filter files for compilation (incremental build logic)
FILES_TO_COMPILE=""
if ${CLEAN_BUILD}; then
    FILES_TO_COMPILE="${ALL_SRC_FILES_RAW}"
else
    # Loop through all raw source files to determine which ones need recompilation
    echo "${ALL_SRC_FILES_RAW}" | while IFS= read -r src_file; do
        # Extract base filename and construct object file path
        base_filename=$(basename "${src_file}")
        obj_name="${base_filename%.cpp}.o"
        obj_file="${OBJ_DIR}/${obj_name}"

        # Check if object file does not exist or if source file is newer
        if [ ! -f "${obj_file}" ] || [ "${src_file}" -nt "${obj_file}" ]; then
            FILES_TO_COMPILE+="${src_file}\n" # Add to list if recompilation is needed
        fi
    done
fi

# Remove any trailing newline from the list of files to compile
FILES_TO_COMPILE=$(echo -e "${FILES_TO_COMPILE}" | sed '/^$/d')
echo "Debug: Files to compile:"
echo "${FILES_TO_COMPILE}"

# --- Compilation Phase ---
if [ -z "${FILES_TO_COMPILE}" ] && ! ${CLEAN_BUILD}; then
    echo "No source files need recompilation."
else
    echo "Compiling source files (debug enabled, clang++)..."
    echo "Debug: Starting compilation for the following files:"
    echo "${FILES_TO_COMPILE}"
    # Use xargs for parallel compilation, stopping on the first error.
    # Each compilation command is run in a subshell.
    echo "${FILES_TO_COMPILE}" | xargs -P "${MAX_JOBS}" -I {} bash -c '
        src_file="$1"
        obj_dir="$2"
        errors_file="$3"

        base_filename=$(basename "${src_file}")
        obj_name="${base_filename%.cpp}.o"
        obj_file="${obj_dir}/${obj_name}"

        # Compile the source file:
        # -g: Include debug info
        # -fno-omit-frame-pointer: Ensure frame pointers for easier debugging
        # -std=c++17: Use C++17 standard
        # -I.: Include current directory
        # -I./NewBCPL: Include NewBCPL directory
        # -I./analysis/az_impl: Include analysis/az_impl directory for modular visitors
        # -c: Compile only, do not link
        # -o: Output object file
        # > /dev/null: Redirect stdout (success messages) to null
        # 2>> "${errors_file}": Append stderr (error messages) to the errors file
        if ! clang++ -g -fno-omit-frame-pointer -std=c++17 -I. -I./NewBCPL -I./analysis/az_impl -I./analysis -I./ -I./include -I./HeapManager -I./runtime -c "${src_file}" -o "${obj_file}" 2>> "${errors_file}"; then
            # If compilation fails, print an error message to stderr and exit the subshell.
            # Exiting the subshell will cause xargs to stop processing further arguments.
            echo "Error: Compilation failed for ${src_file}. See ${errors_file} for details." >&2
            exit 1
        fi
    ' _ {} "${OBJ_DIR}" "${ERRORS_FILE}" # Pass OBJ_DIR and ERRORS_FILE as arguments to the subshell
fi

# Check if any compilation errors occurred by checking the size of the errors file
if [ -s "${ERRORS_FILE}" ]; then
    echo "----------------------------------------"
    echo "BUILD FAILED: Errors detected during compilation. See '${ERRORS_FILE}' for details." >&2
    exit 1 # Exit the script with an error code
fi

# --- Linking Phase ---
echo "Linking object files and JIT runtime library (debug enabled, clang++)..."
# Link all object files into the final executable and the JIT runtime library
if ! clang++ -g -std=c++17 -I. -I./include -I./HeapManager -I./runtime -o "${BIN_DIR}/NewBCPL" "${OBJ_DIR}"/*.o ./libbcpl_runtime_jit.a 2>> "${ERRORS_FILE}"; then
    echo "Error: Linking failed. See '${ERRORS_FILE}' for details." >&2
    exit 1
fi

# --- Codesigning Phase ---
echo "Codesigning the binary with JIT entitlement..."
# Codesign the executable for JIT entitlements.
# Redirect stderr to the errors file.
if ! codesign --entitlements entitlements.plist --sign "-" "${BIN_DIR}/NewBCPL" 2>> "${ERRORS_FILE}"; then
    echo "Error: Codesigning failed. See '${ERRORS_FILE}' for details." >&2
    exit 1
fi

# --- Post-Build Steps ---
echo "Build complete. Executable is at ${BIN_DIR}/NewBCPL"

# Copy the executable to the NewBCPL folder
if ! cp "${BIN_DIR}/NewBCPL" ../NewBCPL/NewBCPL 2>> "${ERRORS_FILE}"; then
    echo "Error: Failed to copy executable. See '${ERRORS_FILE}' for details." >&2
    exit 1
fi
echo "Executable copied to ./NewBCPL/NewBCPL"

# Compile runtime.c as bcpl_runtime.o in the top folder
# (Removed: runtime build is now handled separately)

rm combined.txt
./combiner.sh . combined.txt

echo "----------------------------------------"
echo "Build process finished successfully."
