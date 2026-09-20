#!/bin/bash
# Common preamble sourced by the app repos' scripts: error handling with a
# keep-window-open prompt, pipefail, toolchain selection, cwd at the repo root.

handle_error() {
    echo "An error occurred on line $1"
    read -p "Press enter to continue"
    exit 1
}

trap 'handle_error $LINENO' ERR
set -e -o pipefail

case "$(uname)" in
    Darwin)               TOOLCHAIN="xcode" ;;
    Linux)                TOOLCHAIN="ninja-clang" ;;
    MINGW*|MSYS*|CYGWIN*) TOOLCHAIN="vs" ;;
    *)                    echo "Unsupported platform: $(uname)"; exit 1 ;;
esac

# This file lives at Source/ultra-shared/scripts inside the app repos
cd "$(dirname "${BASH_SOURCE[0]}")/../../.."

# The mac and the Linux box build the shared Windows tree, their build trees stay on the local disk
if [ "$TOOLCHAIN" = "vs" ]; then
    BUILD_DIR="$PWD/Builds/vs"
    LOG_DIR="$PWD/Builds/logs"
else
    BUILD_DIR="$HOME/builds/$(basename "$PWD")"
    LOG_DIR="$BUILD_DIR/logs"
fi
mkdir -p "$LOG_DIR"
