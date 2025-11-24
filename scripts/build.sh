#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

configure_toolchain() {
    if [ -f "$ROOT_DIR/toolchain.yml" ]; then
        TOOLCHAIN_NAME=$(grep "^toolchain:" "$ROOT_DIR/toolchain.yml" | awk '{print $2}')
        if [ -z "$TOOLCHAIN_NAME" ]; then
            echo "Error: Could not parse toolchain name from toolchain.yml" >&2
            exit 1
        fi
    else
        echo "Error: toolchain.yml not found" >&2
        exit 1
    fi

    TOOLCHAIN_DIR="$ROOT_DIR/../../../toolchains/$TOOLCHAIN_NAME"
    TOOLCHAIN_ENV="$TOOLCHAIN_DIR/env.sh"
    if [ ! -f "$TOOLCHAIN_ENV" ]; then
        echo "Error: Toolchain env.sh not found at $TOOLCHAIN_ENV"
        exit 1
    fi

    if  ! source "$TOOLCHAIN_ENV"; then
        exit 1
    fi
}

BUILD_TYPE=$1
if [ -z "$BUILD_TYPE" ] || [ "$BUILD_TYPE" != "native" ] && [ "$BUILD_TYPE" != "cross" ]; then
    echo "Error: Invalid or missing build type. Use 'native' or 'cross'." >&2
    exit 1
fi

if [ "$BUILD_TYPE" == "cross" ]; then
    configure_toolchain
fi

BUILD_DIR="build-$BUILD_TYPE"
LOG_FILE="$BUILD_DIR/build.log"

mkdir -p "$BUILD_DIR"

{
    echo "Build started at $(date)"
    if [ "$BUILD_TYPE" == "cross" ]; then
        echo "Using toolchain: $TOOLCHAIN_NAME"
    else
        echo "Native build"
    fi
    echo "Build directory: $BUILD_DIR"

    if [ "$BUILD_TYPE" == "cross" ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN_FILE"
    else
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    fi

    CMAKE_EXIT=$?
    if [ $CMAKE_EXIT -ne 0 ]; then
        echo "CMake configuration failed with exit code $CMAKE_EXIT" >&2
        echo "Build completed at $(date)"
        exit $CMAKE_EXIT
    fi

    cmake --build "$BUILD_DIR" -- -j"$(nproc)"
    BUILD_EXIT=$?

    echo "Build log saved to $ROOT_DIR/$LOG_FILE"
    echo "Build completed at $(date)"
    exit $BUILD_EXIT
} 2>&1 | tee "$LOG_FILE"

# Capture the exit code from the subshell
exit "${PIPESTATUS[0]}"