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
