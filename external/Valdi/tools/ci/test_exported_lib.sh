#!/bin/bash

# Build helloworld exported aar and inspect contents to make sure that valdimodules and libs are there

# It is not possible to do this from pybuild or run autopilot tests because of workspace limitations

set -e
set -x

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"
ROOT_DIR="$(cd "$SCRIPT_DIR/../../../../"; pwd)"
OPEN_SOURCE_DIR="$(cd "$SCRIPT_DIR/../../"; pwd)"

pushd "$ROOT_DIR"

# Build targets
echo ""
echo "Building HelloWorld exported AAR and standalone app..."
bzl build \
    @valdi//apps/helloworld:hello_world_export_android \
    @valdi//apps/helloworld/standalone_app:hello_world_standalone \
    --copt=-DANDROID_WITH_JNI \
    --repo_env=VALDI_PLATFORM_DEPENDENCIES=android \
    --fat_apk_cpu=arm64-v8a \
    --android_cpu=arm64-v8a

echo "[OK] Build completed successfully"

# Find and verify AAR contents
AAR_PATH="$ROOT_DIR/bazel-bin/external/_main~local_repos~valdi/apps/helloworld/hello_world_export_android.aar"

if [ ! -f "$AAR_PATH" ]; then
    echo "[ERROR] AAR not found at: $AAR_PATH"
    exit 1
fi

echo ""
echo "AAR found at: $AAR_PATH"
echo "AAR size: $(du -h "$AAR_PATH" | cut -f1)"

# Verify AAR contents
echo ""
echo "Verifying AAR contents..."

# Expected modules (should have both .valdimodule and .map.json)
EXPECTED_MODULES=(
    "coreutils"
    "hello_world"
    "jasmine"
    "source_map"
    "valdi_core"
    "valdi_tsx"
)

# Expected JNI library
EXPECTED_LIBS=(
    "jni/arm64-v8a/libhello_world_export.so"
)

MISSING_FILES=()

# Check for valdimodule files
echo "Checking .valdimodule files..."
for module in "${EXPECTED_MODULES[@]}"; do
    file="assets/${module}.valdimodule"
    if unzip -l "$AAR_PATH" | grep -q "$file"; then
        echo "[OK] Found: $file"
    else
        echo "[MISSING] $file"
        MISSING_FILES+=("$file")
    fi
done

# Check for sourcemap files
echo ""
echo "Checking .map.json files (sourcemaps)..."
for module in "${EXPECTED_MODULES[@]}"; do
    file="assets/${module}.map.json"
    if unzip -l "$AAR_PATH" | grep -q "$file"; then
        echo "[OK] Found: $file"
    else
        echo "[MISSING] $file"
        MISSING_FILES+=("$file")
    fi
done

# Check for native libraries
echo ""
echo "Checking JNI libraries..."
for lib in "${EXPECTED_LIBS[@]}"; do
    if unzip -l "$AAR_PATH" | grep -q "$lib"; then
        echo "[OK] Found: $lib"
    else
        echo "[MISSING] $lib"
        MISSING_FILES+=("$lib")
    fi
done

# List all assets and JNI files for debugging
echo ""
echo "All assets in AAR:"
unzip -l "$AAR_PATH" | grep "assets/" || echo "  (none)"

echo ""
echo "All JNI libraries in AAR:"
unzip -l "$AAR_PATH" | grep "jni/" || echo "  (none)"

# Final verdict
if [ ${#MISSING_FILES[@]} -gt 0 ]; then
    echo ""
    echo "================================================================"
    echo "[FAILED] AAR is missing required files"
    echo "================================================================"
    echo "Missing files:"
    for file in "${MISSING_FILES[@]}"; do
        echo "  - $file"
    done
    popd > /dev/null
    exit 1
fi

echo ""
echo "================================================================"
echo "[PASSED] All required files present in AAR"
echo "================================================================"
echo "The valdi_exported_library correctly packages:"
echo "  - ${#EXPECTED_MODULES[@]} .valdimodule files (${EXPECTED_MODULES[*]})"
echo "  - ${#EXPECTED_MODULES[@]} .map.json sourcemap files"
echo "  - Native library (libhello_world_export.so)"

popd

